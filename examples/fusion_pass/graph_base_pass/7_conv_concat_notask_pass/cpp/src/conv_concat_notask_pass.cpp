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
#include <set>
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

// 编译选项key：芯片版本，与 ge_common_api_types.h 中 ge::SOC_VERSION 保持一致
constexpr const char *kOptionSocVersion = "ge.socVersion";
// mc62版本芯片的soc version前缀
constexpr const char *kMc62SocVersionPrefix = "MC62";

// 各conv输出张量大小需满足32B对齐，保证其在concat连续内存中的起始地址32B对齐
constexpr int64_t kBytesAligned = 32;

// 数据类型对应的字节大小
constexpr int64_t kBytes16 = 2;
constexpr int64_t kBytes32 = 4;
constexpr int64_t kBytes64 = 8;

// C 轴对应的序号
constexpr int64_t kCNhwcIndex = 3;

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

// 芯片版本校验：从编译选项中获取soc version，仅mc62版本芯片返回true
bool IsMc62SocVersion(const CustomPassContext &custom_context) {
  AscendString soc_version;
  if (custom_context.GetOptionValue(kOptionSocVersion, soc_version) != GRAPH_SUCCESS) {
    cout << "Failed to get soc version from options, skip." << endl;
    return false;
  }
  const char *soc_version_str = soc_version.GetString();
  if (soc_version_str == nullptr || string(soc_version_str).rfind(kMc62SocVersionPrefix, 0) != 0) {
    cout << "Soc version is not MC62, skip." << endl;
    return false;
  }
  return true;
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

// 计算从指定维度（含）起累乘到末维的slice字节数，不含该维度之前的维度（如batch维N）：
// 多batch下conv在concat输出连续内存中的基地址偏移按“单个batch元素”的slice大小计算，
// 批间排布由kernel按total_shape推导的跳写步长承担，
// 与GE内置ConcatCOptimizeFusionPass::CalSliceOffset、local engine GetConcatOffset口径一致：
// shape中存在非正维度（动态shape）、维度非法或数据类型不支持时返回-1
int64_t GetSliceBytesFromDim(const TensorDesc &tensor_desc, int64_t dim) {
  int64_t type_size = GetDataTypeSize(tensor_desc.GetDataType());
  if (type_size == 0) {
    return -1;
  }
  auto dims = tensor_desc.GetShape().GetDims();
  if (dim < 0 || static_cast<size_t>(dim) >= dims.size()) {
    return -1;
  }
  int64_t element_cnt = 1;
  for (size_t i = static_cast<size_t>(dim); i < dims.size(); ++i) {
    if (dims[i] <= 0) {
      return -1;
    }
    element_cnt *= dims[i];
  }
  return element_cnt * type_size;
}

// 获取C轴的维度索引，仅支持NCHW（C轴为dim 1）/NHWC（C轴为最内层dim 3）格式，其他格式返回false
bool GetCAxisIndex(Format format, int64_t &c_axis_index) {
  if (format == FORMAT_NCHW) {
    c_axis_index = 1;
    return true;
  }
  if (format == FORMAT_NHWC) {
    c_axis_index = kCNhwcIndex;
    return true;
  }
  return false;
}

// 计算conv输出C维度大小字节数（C轴大小×dtype大小），仅支持NCHW/NHWC格式：
// 1. NCHW：C轴为dim 1
// 2. NHWC：C轴为最内层dim 3
// 格式非NCHW/NHWC、维度数不符合、C轴维度非正值（动态shape）或数据类型不支持时返回-1
int64_t GetCAxisAlignBytes(const TensorDesc &tensor_desc) {
  const int64_t type_size = GetDataTypeSize(tensor_desc.GetDataType());
  if (type_size == 0) {
    return -1;
  }
  int64_t c_axis_index;
  if (!GetCAxisIndex(tensor_desc.GetFormat(), c_axis_index)) {
    return -1;
  }
  auto dims = tensor_desc.GetShape().GetDims();
  if (dims.size() <= static_cast<size_t>(c_axis_index) || dims[c_axis_index] <= 0) {
    return -1;
  }
  return dims[c_axis_index] * type_size;
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

// 对齐校验：conv输出format需为NCHW或NHWC，且C维度大小（C轴大小×dtype大小，含义见GetCAxisAlignBytes说明）
// 为32B的整数倍。conv输出张量字节数为C维度大小的整数倍（N×H×W×C×d），conv输出在concat连续内存中按序排布，
// 各输入起始地址等于前序各输入字节数之和，因此C维度大小32B对齐可保证各输入起始地址32B对齐
// （强于GE内置CheckPhonyConcatInputIs32Align的张量大小32B对齐约束）
bool IsConvOutputCAxisAligned(const vector<pair<GNodePtr, int32_t>> &conv_inputs) {
  for (const auto &[conv_node, out_port] : conv_inputs) {
    TensorDesc conv_out_desc;
    if (conv_node->GetOutputDesc(out_port, conv_out_desc) != GRAPH_SUCCESS) {
      return false;
    }
    int64_t c_axis_align_bytes = GetCAxisAlignBytes(conv_out_desc);
    if (c_axis_align_bytes <= 0 || c_axis_align_bytes % kBytesAligned != 0) {
      return false;
    }
  }
  return true;
}

// 校验concat_dim与conv输出的C轴对应：所有conv输入的C轴索引需与concat_dim一致。
// concat_dim支持负索引（从最后一维起算，-1为最后一维）：NCHW格式C轴为dim 1（负索引为-3），
// NHWC格式C轴为最内层dim 3（负索引为-1）
bool IsConcatDimOnCAxis(const vector<pair<GNodePtr, int32_t>> &conv_inputs, int64_t concat_dim) {
  for (const auto &[conv_node, out_port] : conv_inputs) {
    TensorDesc conv_out_desc;
    if (conv_node->GetOutputDesc(out_port, conv_out_desc) != GRAPH_SUCCESS) {
      return false;
    }
    int64_t c_axis_index;
    if (!GetCAxisIndex(conv_out_desc.GetFormat(), c_axis_index)) {
      return false;
    }
    const int64_t dim_cnt = static_cast<int64_t>(conv_out_desc.GetShape().GetDims().size());
    if (concat_dim != c_axis_index && concat_dim != c_axis_index - dim_cnt) {
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
// 1. _output_offset_list_for_continuous：各输出在concat输出连续内存中相对于第1个conv输出地址的字节偏移。
// 多batch下偏移按“单个batch元素”从真实concat轴到末维的slice大小累加，不含batch维N：
// conv的输出基地址只负责批内concat方向偏移，批间排布由kernel按total_shape推导的跳写步长承担
// （第n个batch写在 基地址+offset+n*单batch总大小 处），与GE内置跳写机制口径一致
// 2. total_shape / valid_shape：conv输出所在concat输出的完整shape与conv自身输出的有效shape
graphStatus SetAttrsForConvs(const GNode &concat_node, const vector<pair<GNodePtr, int32_t>> &conv_inputs,
                             int64_t concat_dim) {
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
    int64_t slice_bytes = GetSliceBytesFromDim(conv_out_desc, concat_dim);
    if (slice_bytes <= 0) {
      cout << "Conv2d output slice size is invalid." << endl;
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
    offset += slice_bytes;
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
// 1. 逐图遍历（根图与各子图分别处理），并通过图内节点名集合校验conv输入与concat同图，
//    跨图数据依赖场景（Concat为子图节点而输入conv位于根图）会触发未知错误，直接跳过；
// 2. 给Concat设置 _no_task/_output_reuse_input/_no_padding_continuous_input/_reuse_input_on_dim_index 属性，
//    内存分配时Concat输出不单独分配内存，运行时不生成硬件Task；
// 3. 给每个Conv2D设置 _output_offset_list_for_continuous/total_shape/valid_shape 私有属性，
//    描述其输出在Concat输出连续内存中的字节偏移及完整/有效shape信息。
// 注意：仅针对静态shape、conv输出独占消费、conv输出为NCHW/NHWC格式且C维度大小32B对齐、
// concat轴与conv输出C轴对应的简单场景，实际使用时需结合内存校验确保地址连续性。
// 校验conv输入与concat同图：conv输入的节点名需在当前图的直接节点名集合中。
// 当Concat为子图节点而输入conv位于根图（跨图数据依赖）时，属性标记会触发未知错误，需跳过该场景
bool IsConvInputsInSameGraph(const vector<pair<GNodePtr, int32_t>> &conv_inputs, const set<string> &graph_node_names) {
  for (const auto &[conv_node, out_port] : conv_inputs) {
    AscendString conv_name;
    if (conv_node->GetName(conv_name) != GRAPH_SUCCESS ||
        graph_node_names.find(conv_name.GetString()) == graph_node_names.end()) {
      return false;
    }
  }
  return true;
}

// 逐图处理单张图（根图或子图）中的conv2d+concat pattern
graphStatus ProcessConvConcatInGraph(const GraphPtr &graph, CustomPassContext &custom_context) {
  // 建立当前图直接节点的名字集合，用于conv输入的同图校验
  set<string> graph_node_names;
  for (const auto &node : graph->GetDirectNode()) {
    AscendString node_name;
    if (node.GetName(node_name) == GRAPH_SUCCESS) {
      graph_node_names.emplace(node_name.GetString());
    }
  }
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
    // 2. 校验conv输入与concat同图（跨图数据依赖场景会触发未知错误，直接跳过）
    if (!IsConvInputsInSameGraph(conv_inputs, graph_node_names)) {
      cout << "Conv inputs of concat are not in the same graph, skip." << endl;
      continue;
    }
    // 3. 校验conv输出为NCHW/NHWC格式且C维度大小满足32B对齐（concat连续内存中各输入起始地址对齐的前提）
    if (!IsConvOutputCAxisAligned(conv_inputs)) {
      cout << "Conv2d output format is not supported or C dim size is not 32B aligned, skip." << endl;
      continue;
    }
    // 4. 计算真实的concat轴
    int64_t concat_dim = -1;
    if (!GetRealConcatDim(node, concat_dim)) {
      cout << "Node shape does not match concat pattern, skip." << endl;
      continue;
    }
    // 5. 校验concat_dim与conv输出的C轴对应（NCHW为dim 1或-3，NHWC为dim 3或-1）
    if (!IsConcatDimOnCAxis(conv_inputs, concat_dim)) {
      cout << "Concat dim does not match the C axis of conv output, skip." << endl;
      continue;
    }
    // 6. 设置Conv2d私有属性（先设置conv侧属性，失败时保证concat未被误标记）
    if (SetAttrsForConvs(node, conv_inputs, concat_dim) != GRAPH_SUCCESS) {
      custom_context.SetErrorMessage("Set attrs of conv2d nodes failed.");
      return FAILED;
    }
    // 7. 设置Concat属性
    if (SetAttrsForConcat(node, concat_dim) != GRAPH_SUCCESS) {
      custom_context.SetErrorMessage("Set attrs of concat node failed.");
      return FAILED;
    }
  }
  return GRAPH_SUCCESS;
}

// 递归处理图及其所有子图（visited_graphs按图名去重，防止环）
graphStatus ProcessGraphAndSubgraphs(const GraphPtr &graph, CustomPassContext &custom_context,
                                     set<string> &visited_graphs) {
  AscendString graph_name;
  if (graph->GetName(graph_name) != GRAPH_SUCCESS) {
    cout << "Get graph name failed." << endl;
    return FAILED;
  }
  if (!visited_graphs.emplace(graph_name.GetString()).second) {
    return GRAPH_SUCCESS;
  }
  if (ProcessConvConcatInGraph(graph, custom_context) != GRAPH_SUCCESS) {
    return FAILED;
  }
  for (const auto &subgraph : graph->GetAllSubgraphs()) {
    if (ProcessGraphAndSubgraphs(subgraph, custom_context, visited_graphs) != GRAPH_SUCCESS) {
      return FAILED;
    }
  }
  return GRAPH_SUCCESS;
}

graphStatus ConvConcatNotaskPass(GraphPtr &graph, CustomPassContext &custom_context) {
  cout << "ConvConcatNotaskPass begin." << endl;
  // 0. 芯片版本校验：仅mc62版本芯片执行本pass，其他芯片直接跳过
  if (!IsMc62SocVersion(custom_context)) {
    cout << "ConvConcatNotaskPass end." << endl;
    return GRAPH_SUCCESS;
  }
  // 逐图遍历：根图与各子图分别处理，conv与concat仅在各自图内匹配
  set<string> visited_graphs;
  if (ProcessGraphAndSubgraphs(graph, custom_context, visited_graphs) != GRAPH_SUCCESS) {
    return FAILED;
  }
  cout << "ConvConcatNotaskPass end." << endl;
  return GRAPH_SUCCESS;
}

REGISTER_CUSTOM_PASS("ConvConcatNotaskPass")
    .CustomPassFn(ConvConcatNotaskPass)
    .Stage(CustomPassStage::kAfterOriginGraphOptimize);
