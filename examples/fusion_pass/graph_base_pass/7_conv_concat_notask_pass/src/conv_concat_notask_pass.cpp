/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include <iostream>
#include <vector>
#include "register_custom_pass.h"

using namespace std;
using namespace ge;

namespace {
// concat 侧属性，属性名与 GE 内部 ge_attr_define.h 中 ATTR_NAME_* 定义保持一致
constexpr const char *kAttrNameNoTask = "_no_task";                       // ATTR_NAME_NOTASK
constexpr const char *kAttrNameOutputReuseInput = "_output_reuse_input";  // ATTR_NAME_OUTPUT_REUSE_INPUT
constexpr const char *kAttrNameNoPaddingContinuousInput =
    "_no_padding_continuous_input";  // ATTR_NAME_NOPADDING_CONTINUOUS_INPUT
constexpr const char *kAttrNameReuseInputOnDimIndex =
    "_reuse_input_on_dim_index";  // ATTR_NAME_REUSE_INPUT_ON_DIM_INDEX
// conv2d 侧属性
constexpr const char *kAttrNameOutputOffsetListForContinuous =
    "_output_offset_list_for_continuous";                   // ATTR_NAME_OUTPUT_OFFSET_LIST_FOR_CONTINUOUS
constexpr const char *kAttrNameTotalShape = "total_shape";  // 私有属性：conv 输出所在 concat 输出的完整 shape
constexpr const char *kAttrNameValidShape = "valid_shape";  // 私有属性：conv 自身输出的有效 shape

constexpr const char *kOpTypeConv2D = "Conv2D";
constexpr const char *kOpTypeConv2DV2 = "Conv2DV2";
constexpr const char *kOpTypeExtendConv2D = "ExtendConv2D";
constexpr const char *kOpTypeConcatD = "ConcatD";
constexpr const char *kOpTypeConcatV2D = "ConcatV2D";

// 各conv输出张量大小需满足32B对齐，保证其在concat连续内存中的起始地址32B对齐
constexpr int64_t kBytesAligned = 32;

// 数据类型对应的字节大小
constexpr int64_t kBytes16 = 2;
constexpr int64_t kBytes32 = 4;
constexpr int64_t kBytes64 = 8;

bool IsConv2DNode(const GNode &node) {
  AscendString node_type;
  if (node.GetType(node_type) != GRAPH_SUCCESS) {
    return false;
  }
  return node_type == kOpTypeConv2D || node_type == kOpTypeConv2DV2 || node_type == kOpTypeExtendConv2D;
}

bool IsConcatNode(const GNode &node) {
  AscendString node_type;
  if (node.GetType(node_type) != GRAPH_SUCCESS) {
    return false;
  }
  return node_type == kOpTypeConcatD || node_type == kOpTypeConcatV2D;
}

bool IsNoTaskNode(const GNode &node) {
  bool no_task = false;
  return node.GetAttr(kAttrNameNoTask, no_task) == GRAPH_SUCCESS && no_task;
}

// 数据类型对应的字节大小，不支持的类型返回0
int64_t GetDataTypeSize(DataType data_type) {
  switch (data_type) {
    case DT_BOOL:
    case DT_INT8:
    case DT_UINT8:
      return 1;
    case DT_FLOAT16:
    case DT_BF16:
    case DT_INT16:
    case DT_UINT16:
      return kBytes16;
    case DT_FLOAT:
    case DT_INT32:
    case DT_UINT32:
      return kBytes32;
    case DT_DOUBLE:
    case DT_INT64:
    case DT_UINT64:
      return kBytes64;
    default:
      return 0;
  }
}

// 计算张量字节数，shape中存在非正维度（动态shape）或数据类型不支持时返回-1
int64_t GetTensorBytes(const TensorDesc &tensor_desc) {
  int64_t type_size = GetDataTypeSize(tensor_desc.GetDataType());
  if (type_size == 0) {
    return -1;
  }
  int64_t element_cnt = 1;
  for (auto dim : tensor_desc.GetShape().GetDims()) {
    if (dim <= 0) {
      return -1;
    }
    element_cnt *= dim;
  }
  return element_cnt * type_size;
}

// 参考 GE 内置 StridedWriteOptimizer::GetRealConcatDim：
// 比较 concat 第0个输入与输出的shape，第一个取值不同的维度即为真实的concat轴
bool GetRealConcatDim(const GNode &concat_node, int64_t &concat_dim) {
  TensorDesc input_desc;
  TensorDesc output_desc;
  if (concat_node.GetInputDesc(0, input_desc) != GRAPH_SUCCESS ||
      concat_node.GetOutputDesc(0, output_desc) != GRAPH_SUCCESS) {
    return false;
  }
  auto input_dims = input_desc.GetShape().GetDims();
  auto output_dims = output_desc.GetShape().GetDims();
  if (input_dims.empty() || input_dims.size() != output_dims.size()) {
    return false;
  }
  for (size_t i = 0; i < input_dims.size(); ++i) {
    if (input_dims[i] != output_dims[i]) {
      concat_dim = static_cast<int64_t>(i);
      return true;
    }
  }
  return false;
}

// pattern校验：concat的所有输入均来自不同的Conv2D/Conv2DV2/ExtendConv2D节点，且对应conv输出仅被该concat消费
bool MatchConvConcatPattern(const GNode &concat_node, vector<pair<GNodePtr, int32_t>> &conv_inputs) {
  for (size_t i = 0; i < concat_node.GetInputsSize(); ++i) {
    auto [in_node, out_port] = concat_node.GetInDataNodesAndPortIndexs(static_cast<int32_t>(i));
    if (in_node == nullptr || !IsConv2DNode(*in_node)) {
      return false;
    }
    for (const auto &matched : conv_inputs) {
      if (matched.first == in_node) {  // 同一个Conv2D输出重复作为多个输入时不支持
        return false;
      }
    }
    if (in_node->GetOutDataNodesAndPortIndexs(out_port).size() != 1) {  // conv输出被其他节点消费时不能复用其内存
      return false;
    }
    conv_inputs.emplace_back(in_node, out_port);
  }
  return !conv_inputs.empty();
}

// 对齐校验：每个conv输出在C轴及后续维度上连续排布，其张量字节数需为32B的整数倍。
// conv输出在concat连续内存中按序排布，各输入起始地址等于前序输入大小之和，
// 因此每个conv输出大小32B对齐是各输入起始地址32B对齐的充分条件
// （与GE内置CheckPhonyConcatInputIs32Align的32B对齐约束一致）
bool IsConvOutputBytesAligned(const vector<pair<GNodePtr, int32_t>> &conv_inputs) {
  for (const auto &[conv_node, out_port] : conv_inputs) {
    TensorDesc conv_out_desc;
    if (conv_node->GetOutputDesc(out_port, conv_out_desc) != GRAPH_SUCCESS) {
      return false;
    }
    int64_t conv_out_bytes = GetTensorBytes(conv_out_desc);
    if (conv_out_bytes <= 0 || conv_out_bytes % kBytesAligned != 0) {
      return false;
    }
  }
  return true;
}

graphStatus SetAttrsForConcat(const GNode &concat_node, int64_t concat_dim) {
  bool no_task = true;
  bool output_reuse_input = true;
  bool no_padding_continuous_input = true;
  if (concat_node.SetAttr(kAttrNameNoTask, no_task) != GRAPH_SUCCESS ||
      concat_node.SetAttr(kAttrNameOutputReuseInput, output_reuse_input) != GRAPH_SUCCESS ||
      concat_node.SetAttr(kAttrNameNoPaddingContinuousInput, no_padding_continuous_input) != GRAPH_SUCCESS ||
      concat_node.SetAttr(kAttrNameReuseInputOnDimIndex, concat_dim) != GRAPH_SUCCESS) {
    cout << "Set attrs of concat node failed." << endl;
    return FAILED;
  }
  return GRAPH_SUCCESS;
}

// 为每个Conv2D设置私有属性：
// 1. _output_offset_list_for_continuous：各输出在concat输出连续内存中相对于第1个conv输出地址的字节偏移
// 2. total_shape / valid_shape：conv输出所在concat输出的完整shape与conv自身输出的有效shape
graphStatus SetAttrsForConvs(const GNode &concat_node, const vector<pair<GNodePtr, int32_t>> &conv_inputs) {
  TensorDesc concat_out_desc;
  if (concat_node.GetOutputDesc(0, concat_out_desc) != GRAPH_SUCCESS) {
    cout << "Get output desc of concat node failed." << endl;
    return FAILED;
  }
  vector<int64_t> total_shape = concat_out_desc.GetShape().GetDims();
  int64_t offset = 0;
  for (const auto &[conv_node, out_port] : conv_inputs) {
    TensorDesc conv_out_desc;
    if (conv_node->GetOutputDesc(out_port, conv_out_desc) != GRAPH_SUCCESS) {
      cout << "Get output desc of conv2d node failed." << endl;
      return FAILED;
    }
    int64_t conv_out_bytes = GetTensorBytes(conv_out_desc);
    if (conv_out_bytes <= 0) {
      cout << "Conv2d output size is invalid." << endl;
      return FAILED;
    }
    vector<int64_t> valid_shape = conv_out_desc.GetShape().GetDims();
    vector<int64_t> offset_list(conv_node->GetOutputsSize(), 0);
    offset_list[out_port] = offset;
    if (conv_node->SetAttr(kAttrNameOutputOffsetListForContinuous, offset_list) != GRAPH_SUCCESS ||
        conv_node->SetAttr(kAttrNameTotalShape, total_shape) != GRAPH_SUCCESS ||
        conv_node->SetAttr(kAttrNameValidShape, valid_shape) != GRAPH_SUCCESS) {
      cout << "Set attrs of conv2d node failed." << endl;
      return FAILED;
    }
    offset += conv_out_bytes;
  }
  return GRAPH_SUCCESS;
}
}  // namespace

// |o>------------------------------------------------------------
// |o>    a         b
// |o>     \       /
// |o>     Conv2D  ExtendConv2D  ==>   conv0_out + conv1_out 按序连续排布
// |o>        \      /                 Concat标记为虚拟算子：不生成Task、输出复用输入内存
// |o>        ConcatD
// |o>------------------------------------------------------------
// 识别说明：本例识别上图中Conv2D/Conv2DV2/ExtendConv2D与ConcatD/ConcatV2D组成的结构，通过属性标记实现Concat零拷贝优化：
// 1. 给Concat设置 _no_task/_output_reuse_input/_no_padding_continuous_input/_reuse_input_on_dim_index 属性，
//    内存分配时Concat输出不单独分配内存，运行时不生成硬件Task；
// 2. 给每个Conv2D设置 _output_offset_list_for_continuous/total_shape/valid_shape 私有属性，
//    描述其输出在Concat输出连续内存中的字节偏移及完整/有效shape信息。
// 注意：仅针对静态shape、conv输出独占消费、conv输出大小32B对齐的简单场景，
// 实际使用时需结合内存校验确保地址连续性。
graphStatus ConvConcatNotaskPass(GraphPtr &graph, CustomPassContext &custom_context) {
  cout << "ConvConcatNotaskPass begin." << endl;
  for (auto &node : graph->GetDirectNode()) {
    if (!IsConcatNode(node) || IsNoTaskNode(node)) {
      continue;
    }
    // 1. 匹配conv2d + concat的pattern
    vector<pair<GNodePtr, int32_t>> conv_inputs;
    if (!MatchConvConcatPattern(node, conv_inputs)) {
      cout << "Node is not a conv2d + concat pattern, skip." << endl;
      continue;
    }
    // 2. 校验各conv输出大小满足32B对齐（concat连续内存中各输入起始地址对齐的前提）
    if (!IsConvOutputBytesAligned(conv_inputs)) {
      cout << "Conv2d output bytes are not 32B aligned, skip." << endl;
      continue;
    }
    // 3. 计算真实的concat轴
    int64_t concat_dim = -1;
    if (!GetRealConcatDim(node, concat_dim)) {
      cout << "Node shape does not match concat pattern, skip." << endl;
      continue;
    }
    // 4. 设置Conv2d私有属性（先设置conv侧属性，失败时保证concat未被误标记）
    if (SetAttrsForConvs(node, conv_inputs) != GRAPH_SUCCESS) {
      custom_context.SetErrorMessage("Set attrs of conv2d nodes failed.");
      return FAILED;
    }
    // 5. 设置Concat属性
    if (SetAttrsForConcat(node, concat_dim) != GRAPH_SUCCESS) {
      custom_context.SetErrorMessage("Set attrs of concat node failed.");
      return FAILED;
    }
  }
  cout << "ConvConcatNotaskPass end." << endl;
  return GRAPH_SUCCESS;
}

REGISTER_CUSTOM_PASS("ConvConcatNotaskPass")
    .CustomPassFn(ConvConcatNotaskPass)
    .Stage(CustomPassStage::kAfterOriginGraphOptimize);
