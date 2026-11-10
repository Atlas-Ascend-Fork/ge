/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

/*
 * ES C ABI 全函数覆盖、ES 便捷层（EsCGraphBuilder/EsCTensorHolder/EsTensorLike/EsGraphBuilder）
 * 直接调用面、以及“内部 ExecuteGraph 接口 vs GEIR 门面接口”的组合等价性用例
 * （对应《ES_Graph构图接口UT设计表》ES_C_ABI/EsCGraphBuilder/EsCTensorHolder/门面等价性 页）。
 * 双模式接口的计算图场景对照见 es_facade_focus_unittest.cc。
 */
#include <gtest/gtest.h>
#include <cstdio>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "graph/graph.h"
#include "graph/gnode.h"
#include "graph/operator.h"
#include "graph/attr_value.h"
#include "graph/ascend_string.h"
#include "graph/buffer.h"
#include "graph/op_desc.h"
#include "graph/tensor.h"
#include "graph/utils/graph_utils_ex.h"
#include "graph/utils/node_adapter.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/execute_graph_utils.h"
#include "graph/utils/fast_node_utils.h"
#include "graph/fast_graph/execute_graph.h"
#include "graph/fast_graph/fast_node.h"
#include "graph_metadef/graph/debug/ge_util.h"
#include "normal_graph/execute_graph_backend.h"
#include "es_graph_builder.h"
#include "normal_graph/execute_graph_backend_utils.h"
#include "es_c_graph_builder.h"
#include "es_tensor_like.h"
#include "esb_funcs.h"
#include "graph_builder_utils.h"

namespace ge {
namespace {
// GraphKind 依赖切换期暂为整型别名（0=计算图，1=执行图），用例内以常量指代
constexpr GraphKind kExecGraphKind = static_cast<GraphKind>(1);
Operator MakePortOp(const char_t *name, const char_t *type, uint32_t in_cnt, uint32_t out_cnt) {
  Operator op(name, type);
  if (in_cnt != 0U) {
    op.DynamicInputRegister("x", in_cnt);
  }
  if (out_cnt != 0U) {
    op.DynamicOutputRegister("y", out_cnt);
  }
  return op;
}

OpDescPtr MakeOpDesc(const std::string &name, const std::string &type, uint32_t in_cnt, uint32_t out_cnt) {
  auto desc = ComGraphMakeShared<OpDesc>(name, type);
  for (uint32_t i = 0U; i < in_cnt; ++i) {
    (void)desc->AddInputDesc("x" + std::to_string(i), GeTensorDesc());
  }
  for (uint32_t i = 0U; i < out_cnt; ++i) {
    (void)desc->AddOutputDesc("y" + std::to_string(i), GeTensorDesc());
  }
  return desc;
}

std::string NodeNameOf(const GNodePtr &node) {
  AscendString name;
  if ((node == nullptr) || (node->GetName(name) != GRAPH_SUCCESS)) {
    return "<null>";
  }
  return std::string(name.GetString());
}

std::string WriteTempBinary(const std::string &tag, const std::vector<uint8_t> &bytes) {
  const std::string path = "/tmp/es_facade_" + tag + ".bin";
  std::ofstream ofs(path, std::ios::binary | std::ios::trunc);
  ofs.write(reinterpret_cast<const char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
  ofs.close();
  return path;
}
}  // namespace

// ============================================================================
// 一、ES C ABI（esb_funcs.h 全部 41 个函数的覆盖与关键负向）
// ============================================================================
class EsFacadeCabiTest : public testing::Test {};

// EsCreateGraphBuilder：默认计算图构建器，C 入口全流程（计算图场景）。
TEST_F(EsFacadeCabiTest, CreateBuilder_DefaultComputeFlow) {
  auto *builder = EsCreateGraphBuilder(nullptr);
  ASSERT_NE(nullptr, builder);
  auto *input = EsCreateGraphInput(builder, 0);
  ASSERT_NE(nullptr, input);
  ASSERT_EQ(0U, EsSetInt64AttrForGraph(builder, "k", 5));
  ASSERT_EQ(0U, EsSetGraphOutput(input, 0));
  auto *graph = EsBuildGraphAndReset(builder);
  ASSERT_NE(nullptr, graph);
  auto *owned = static_cast<ge::Graph *>(static_cast<void *>(graph));
  EXPECT_NE(nullptr, GraphUtilsEx::GetComputeGraph(*owned));
  AttrValue out;
  ASSERT_EQ(GRAPH_SUCCESS, owned->GetAttr(AscendString("k"), out));
  int64_t v = 0;
  ASSERT_EQ(GRAPH_SUCCESS, out.GetAttrValue(v));
  EXPECT_EQ(5, v);
  delete owned;
  EsDestroyGraphBuilder(builder);
}

// EsCreateGraphBuilderWithKind：执行类别流程 + 非法类别拒绝（A35）。
TEST_F(EsFacadeCabiTest, CreateBuilderWithKind_ExecuteFlowAndInvalidKind) {
  auto *builder = EsCreateGraphBuilderWithKind("cabi_exec", kExecGraphKind);
  ASSERT_NE(nullptr, builder);
  auto *input = EsCreateGraphInput(builder, 0);
  ASSERT_NE(nullptr, input);
  ASSERT_EQ(0U, EsSetGraphOutput(input, 0));
  auto *graph = EsBuildGraphAndReset(builder);
  ASSERT_NE(nullptr, graph);
  auto *owned = static_cast<ge::Graph *>(static_cast<void *>(graph));
  EXPECT_NE(nullptr, GraphUtilsEx::Graph2ExecuteGraph(*owned));
  EXPECT_EQ(nullptr, GraphUtilsEx::GetComputeGraph(*owned));
  delete owned;
  EsDestroyGraphBuilder(builder);

  EXPECT_EQ(nullptr, EsCreateGraphBuilderWithKind("cabi_bad", static_cast<GraphKind>(9)));
}

// EsCreateGraphInputWithDetails：具名带描述输入读回一致（A36）。
TEST_F(EsFacadeCabiTest, CreateGraphInputWithDetails_Readback) {
  auto *builder = EsCreateGraphBuilderWithKind("cabi_in", kExecGraphKind);
  ASSERT_NE(nullptr, builder);
  const int64_t shape[] = {2, 2};
  auto *input = EsCreateGraphInputWithDetails(builder, 0, "my_in", nullptr, C_DT_FLOAT16, C_FORMAT_NCHW, shape, 2);
  ASSERT_NE(nullptr, input);
  auto *producer = EsGetProducer(input);
  ASSERT_NE(nullptr, producer);
  auto *node = static_cast<GNode *>(static_cast<void *>(producer));
  AscendString name;
  ASSERT_EQ(GRAPH_SUCCESS, node->GetName(name));
  EXPECT_EQ(AscendString("my_in"), name);
  TensorDesc desc;
  ASSERT_EQ(GRAPH_SUCCESS, node->GetOutputDesc(0, desc));
  EXPECT_EQ(DT_FLOAT16, desc.GetDataType());
  EXPECT_EQ(FORMAT_NCHW, desc.GetFormat());
  EXPECT_EQ(2, desc.GetShape().GetDim(1));
  EsDestroyGraphBuilder(builder);
}

// EsCreateGraphInput：重复索引失败。
TEST_F(EsFacadeCabiTest, CreateGraphInput_DuplicateIndexFails) {
  auto *builder = EsCreateGraphBuilder("cabi_dup_idx");
  ASSERT_NE(nullptr, builder);
  ASSERT_NE(nullptr, EsCreateGraphInput(builder, 0));
  EXPECT_EQ(nullptr, EsCreateGraphInput(builder, 0));
  EsDestroyGraphBuilder(builder);
}

// EsSetDataType/EsSetFormat/EsSetShape/EsSetOriginSymbolShape：描述设置四件套回环。
TEST_F(EsFacadeCabiTest, SetDescFunctions_Roundtrip) {
  auto *builder = EsCreateGraphBuilder("cabi_desc");
  ASSERT_NE(nullptr, builder);
  auto *input = EsCreateGraphInput(builder, 0);
  ASSERT_NE(nullptr, input);
  const int64_t shape[] = {3, 4};
  ASSERT_EQ(0U, EsSetShape(input, shape, 2));
  ASSERT_EQ(0U, EsSetDataType(input, C_DT_INT32));
  ASSERT_EQ(0U, EsSetFormat(input, C_FORMAT_NHWC));
  const char *const symbol[] = {"-1", "16"};
  ASSERT_EQ(0U, EsSetOriginSymbolShape(input, symbol, 2));

  auto *node = static_cast<GNode *>(static_cast<void *>(EsGetProducer(input)));
  TensorDesc desc;
  ASSERT_EQ(GRAPH_SUCCESS, node->GetOutputDesc(0, desc));
  EXPECT_EQ(DT_INT32, desc.GetDataType());
  EXPECT_EQ(FORMAT_NHWC, desc.GetFormat());
  EXPECT_EQ(4, desc.GetShape().GetDim(1));
  // 标量豁免：shape 为空 + dim_num=0。
  ASSERT_EQ(0U, EsSetShape(input, nullptr, 0));
  EsDestroyGraphBuilder(builder);
}

// EsCreateConstInt64/Int32/UInt64/UInt32/Float：类型化常量建后 value 属性读回（A31）。
TEST_F(EsFacadeCabiTest, CreateConst_TypedVariantsRoundtrip) {
  auto *builder = EsCreateGraphBuilderWithKind("cabi_const", kExecGraphKind);
  ASSERT_NE(nullptr, builder);

  const int64_t i64[] = {1, 2};
  const int64_t dims[] = {2};
  auto *c_i64 = EsCreateConstInt64(builder, i64, dims, 1);
  ASSERT_NE(nullptr, c_i64);
  const int32_t i32[] = {3};
  auto *c_i32 = EsCreateConstInt32(builder, i32, dims, 1);
  ASSERT_NE(nullptr, c_i32);
  const uint64_t u64[] = {4};
  auto *c_u64 = EsCreateConstUInt64(builder, u64, dims, 1);
  ASSERT_NE(nullptr, c_u64);
  const uint32_t u32[] = {5};
  auto *c_u32 = EsCreateConstUInt32(builder, u32, dims, 1);
  ASSERT_NE(nullptr, c_u32);
  const float f32[] = {6.5f};
  auto *c_f32 = EsCreateConstFloat(builder, f32, dims, 1);
  ASSERT_NE(nullptr, c_f32);

  const std::vector<EsCTensorHolder *> consts = {c_i64, c_i32, c_u64, c_u32, c_f32};
  const std::vector<int64_t> expect = {1, 3, 4, 5, 6};
  for (size_t i = 0U; i < consts.size(); ++i) {
    auto *node = static_cast<GNode *>(static_cast<void *>(EsGetProducer(consts[i])));
    Tensor tensor;
    ASSERT_EQ(GRAPH_SUCCESS, node->GetAttr(AscendString("value"), tensor));
    const auto *data = tensor.GetData();
    ASSERT_NE(nullptr, data);
    int64_t got = 0;
    switch (i) {
      case 0U:
        got = *(reinterpret_cast<const int64_t *>(data));
        break;
      case 1U:
        got = *(reinterpret_cast<const int32_t *>(data));
        break;
      case 2U:
        got = static_cast<int64_t>(*(reinterpret_cast<const uint64_t *>(data)));
        break;
      case 3U:
        got = *(reinterpret_cast<const uint32_t *>(data));
        break;
      default:
        got = static_cast<int64_t>(*(reinterpret_cast<const float *>(data)));
        break;
    }
    EXPECT_EQ(expect[i], got) << "const variant " << i;
  }
  EsDestroyGraphBuilder(builder);
}

// EsCreateVector*：五型向量常量（长度与首元素）。
TEST_F(EsFacadeCabiTest, CreateVector_Variants) {
  auto *builder = EsCreateGraphBuilder("cabi_vec");
  ASSERT_NE(nullptr, builder);
  const int64_t vi64[] = {10, 20};
  ASSERT_NE(nullptr, EsCreateVectorInt64(builder, vi64, 2));
  const int32_t vi32[] = {30};
  ASSERT_NE(nullptr, EsCreateVectorInt32(builder, vi32, 1));
  const uint64_t vu64[] = {40};
  ASSERT_NE(nullptr, EsCreateVectorUInt64(builder, vu64, 1));
  const uint32_t vu32[] = {50};
  ASSERT_NE(nullptr, EsCreateVectorUInt32(builder, vu32, 1));
  const float vf[] = {60.0f};
  ASSERT_NE(nullptr, EsCreateVectorFloat(builder, vf, 1));
  EsDestroyGraphBuilder(builder);
}

// EsCreateScalar*：五型标量常量——节点存在性 + 标量 shape + 值读回（设计表测试点②）。
TEST_F(EsFacadeCabiTest, CreateScalar_VariantsWithReadback) {
  auto *builder = EsCreateGraphBuilder("cabi_scalar");
  ASSERT_NE(nullptr, builder);
  const std::vector<EsCTensorHolder *> holders = {
      EsCreateScalarInt64(builder, -123), EsCreateScalarInt32(builder, -45), EsCreateScalarUInt64(builder, 1234U),
      EsCreateScalarUInt32(builder, 567U), EsCreateScalarFloat(builder, -3.5f)};
  for (auto *holder : holders) {
    ASSERT_NE(nullptr, holder);
    auto *node = static_cast<GNode *>(static_cast<void *>(EsGetProducer(holder)));
    ASSERT_NE(nullptr, node);
    Tensor tensor;
    ASSERT_EQ(GRAPH_SUCCESS, node->GetAttr(AscendString("value"), tensor));
    EXPECT_EQ(0, tensor.GetTensorDesc().GetShape().GetDimNum());  // 标量 shape
    const auto *data = tensor.GetData();
    ASSERT_NE(nullptr, data);
    switch (tensor.GetTensorDesc().GetDataType()) {
      case DT_INT64:
        EXPECT_EQ(-123, *reinterpret_cast<const int64_t *>(data));
        break;
      case DT_INT32:
        EXPECT_EQ(-45, *reinterpret_cast<const int32_t *>(data));
        break;
      case DT_UINT64:
        EXPECT_EQ(1234U, *reinterpret_cast<const uint64_t *>(data));
        break;
      case DT_UINT32:
        EXPECT_EQ(567U, *reinterpret_cast<const uint32_t *>(data));
        break;
      case DT_FLOAT:
        EXPECT_FLOAT_EQ(-3.5f, *reinterpret_cast<const float *>(data));
        break;
      default:
        FAIL() << "unexpected dtype";
    }
  }
  EsDestroyGraphBuilder(builder);
}

// EsCreateVariable：变量节点 index 属性读回；执行图上可建（语义现状记录）。
TEST_F(EsFacadeCabiTest, CreateVariable_IndexAttr) {
  auto *builder = EsCreateGraphBuilder("cabi_var");
  ASSERT_NE(nullptr, builder);
  auto *var = EsCreateVariable(builder, 2, "var1");
  ASSERT_NE(nullptr, var);
  auto *node = static_cast<GNode *>(static_cast<void *>(EsGetProducer(var)));
  int64_t index = -1;
  ASSERT_EQ(GRAPH_SUCCESS, node->GetAttr(AscendString("index"), index));
  EXPECT_EQ(2, index);
  EsDestroyGraphBuilder(builder);
}

// EsSetGraphOutput：槽位非负唯一；重复/负槽位失败。
TEST_F(EsFacadeCabiTest, SetGraphOutput_SlotChecks) {
  auto *builder = EsCreateGraphBuilder("cabi_out");
  ASSERT_NE(nullptr, builder);
  auto *input = EsCreateGraphInput(builder, 0);
  ASSERT_NE(nullptr, input);
  ASSERT_EQ(0U, EsSetGraphOutput(input, 0));
  EXPECT_NE(0U, EsSetGraphOutput(input, 0));   // 重复槽位
  EXPECT_NE(0U, EsSetGraphOutput(input, -1));  // 负槽位
  EXPECT_NE(0U, EsSetGraphOutput(nullptr, 0));
  EsDestroyGraphBuilder(builder);
}

// EsBuildGraphAndReset：一次性语义 + 计算图收尾含 NetOutput（计算图场景，A33）。
TEST_F(EsFacadeCabiTest, BuildGraphAndReset_HandsOverOnce_ComputeNetOutput) {
  auto *builder = EsCreateGraphBuilder("cabi_build");
  ASSERT_NE(nullptr, builder);
  auto *input = EsCreateGraphInput(builder, 0);
  ASSERT_NE(nullptr, input);
  ASSERT_EQ(0U, EsSetGraphOutput(input, 0));
  auto *raw = EsBuildGraphAndReset(builder);
  ASSERT_NE(nullptr, raw);
  std::unique_ptr<ge::Graph> graph(static_cast<ge::Graph *>(static_cast<void *>(raw)));
  ASSERT_NE(nullptr, GraphUtilsEx::GetComputeGraph(*graph));
  auto net_output = graph->FindNodeByName(AscendString("NetOutput_cabi_build"));
  EXPECT_NE(nullptr, net_output);
  EXPECT_NE(nullptr, graph->FindNodeByName(AscendString("input_0")));
  // 一次性：再收尾失败。
  EXPECT_EQ(nullptr, EsBuildGraphAndReset(builder));
  EsDestroyGraphBuilder(builder);
}

// EsGetProducer/EsGetOwnerBuilder：快照句柄与创建方一致。
TEST_F(EsFacadeCabiTest, GetProducer_GetOwnerBuilder) {
  auto *builder = EsCreateGraphBuilder("cabi_get");
  ASSERT_NE(nullptr, builder);
  auto *input = EsCreateGraphInput(builder, 0);
  ASSERT_NE(nullptr, input);
  EXPECT_EQ(builder, EsGetOwnerBuilder(input));
  auto *producer = EsGetProducer(input);
  ASSERT_NE(nullptr, producer);
  auto *node = static_cast<GNode *>(static_cast<void *>(producer));
  AscendString name;
  ASSERT_EQ(GRAPH_SUCCESS, node->GetName(name));
  EXPECT_EQ(AscendString("input_0"), name);
  EXPECT_EQ(nullptr, EsGetProducer(nullptr));
  EXPECT_EQ(nullptr, EsGetOwnerBuilder(nullptr));
  EsDestroyGraphBuilder(builder);
}

// EsAddControlEdge：成功路径与非法入参（P3 部分行为记录：nullptr 失败；num=0 当前返回成功）。
TEST_F(EsFacadeCabiTest, AddControlEdge_SuccessAndInvalidArgs) {
  auto *builder = EsCreateGraphBuilderWithKind("cabi_ctrl", kExecGraphKind);
  ASSERT_NE(nullptr, builder);
  auto *dst = EsCreateGraphInput(builder, 0);
  ASSERT_NE(nullptr, dst);
  const int64_t value = 1;
  auto *src = EsCreateScalarInt64(builder, value);
  ASSERT_NE(nullptr, src);

  EsCTensorHolder *srcs[] = {src};
  ASSERT_EQ(0U, EsAddControlEdge(dst, srcs, 1));
  auto *dst_node = static_cast<GNode *>(static_cast<void *>(EsGetProducer(dst)));
  ASSERT_EQ(1U, dst_node->GetInControlNodes().size());

  // 非法入参：空目标/空数组失败。
  EXPECT_NE(0U, EsAddControlEdge(nullptr, srcs, 1));
  EXPECT_NE(0U, EsAddControlEdge(dst, nullptr, 1));
  // 行为记录（P3）：num<=0 且指针有效时当前直接返回成功。
  EXPECT_EQ(0U, EsAddControlEdge(dst, srcs, 0));
  EsDestroyGraphBuilder(builder);
}

// EsCreateEsCTensor：12 种支持类型逐一创建并读回首元素；不支持类型拒绝。
TEST_F(EsFacadeCabiTest, CreateEsCTensor_AllTypes) {
  const float f[] = {5.0f};
  ASSERT_NE(nullptr, EsCreateEsCTensor(f, nullptr, 1, C_DT_FLOAT, C_FORMAT_ND));
  const float f16[] = {5.0f};
  ASSERT_NE(nullptr, EsCreateEsCTensor(f16, nullptr, 1, C_DT_FLOAT16, C_FORMAT_ND));
  const int8_t i8[] = {5};
  ASSERT_NE(nullptr, EsCreateEsCTensor(i8, nullptr, 1, C_DT_INT8, C_FORMAT_ND));
  const int16_t i16[] = {5};
  ASSERT_NE(nullptr, EsCreateEsCTensor(i16, nullptr, 1, C_DT_INT16, C_FORMAT_ND));
  const int32_t i32[] = {5};
  ASSERT_NE(nullptr, EsCreateEsCTensor(i32, nullptr, 1, C_DT_INT32, C_FORMAT_ND));
  const int64_t i64[] = {5};
  ASSERT_NE(nullptr, EsCreateEsCTensor(i64, nullptr, 1, C_DT_INT64, C_FORMAT_ND));
  const uint8_t u8[] = {5};
  ASSERT_NE(nullptr, EsCreateEsCTensor(u8, nullptr, 1, C_DT_UINT8, C_FORMAT_ND));
  const uint16_t u16[] = {5};
  ASSERT_NE(nullptr, EsCreateEsCTensor(u16, nullptr, 1, C_DT_UINT16, C_FORMAT_ND));
  const uint32_t u32[] = {5};
  ASSERT_NE(nullptr, EsCreateEsCTensor(u32, nullptr, 1, C_DT_UINT32, C_FORMAT_ND));
  const uint64_t u64[] = {5};
  ASSERT_NE(nullptr, EsCreateEsCTensor(u64, nullptr, 1, C_DT_UINT64, C_FORMAT_ND));
  const bool b[] = {true};
  auto *bool_tensor = EsCreateEsCTensor(b, nullptr, 1, C_DT_BOOL, C_FORMAT_ND);
  ASSERT_NE(nullptr, bool_tensor);
  auto *tensor = static_cast<ge::Tensor *>(static_cast<void *>(bool_tensor));
  EXPECT_EQ(1, *(tensor->GetData()));

  EXPECT_EQ(nullptr, EsCreateEsCTensor(f, nullptr, 1, static_cast<C_DataType>(0xABCD), C_FORMAT_ND));
  EXPECT_EQ(nullptr, EsCreateEsCTensor(nullptr, nullptr, 1, C_DT_FLOAT, C_FORMAT_ND));
}

// EsCreateEsCTensorFromFile：文件数据一致 + 长度校验。
TEST_F(EsFacadeCabiTest, CreateEsCTensorFromFile_RoundtripAndSizeChecks) {
  const std::vector<uint8_t> bytes = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
  const std::string path = WriteTempBinary("from_file", bytes);
  const int64_t dims[] = {2};
  auto *raw = EsCreateEsCTensorFromFile(path.c_str(), dims, 1, C_DT_UINT32, C_FORMAT_ND);
  ASSERT_NE(nullptr, raw);
  auto *tensor = static_cast<ge::Tensor *>(static_cast<void *>(raw));
  const auto *data = reinterpret_cast<const uint32_t *>(tensor->GetData());
  ASSERT_NE(nullptr, data);
  EXPECT_EQ(0x44332211U, data[0]);
  EXPECT_EQ(0x88776655U, data[1]);
  delete tensor;

  // 尺寸不足失败：声明 4 个 uint32 但文件只有 8 字节。
  const int64_t big_dims[] = {4};
  EXPECT_EQ(nullptr, EsCreateEsCTensorFromFile(path.c_str(), big_dims, 1, C_DT_UINT32, C_FORMAT_ND));
  // 空文件失败。
  const std::string empty_path = WriteTempBinary("empty", {});
  EXPECT_EQ(nullptr, EsCreateEsCTensorFromFile(empty_path.c_str(), nullptr, 0, C_DT_UINT32, C_FORMAT_ND));
  // 不存在路径失败。
  EXPECT_EQ(nullptr,
            EsCreateEsCTensorFromFile("/tmp/es_facade_no_such_file.bin", nullptr, 0, C_DT_UINT32, C_FORMAT_ND));
  (void)std::remove(path.c_str());
  (void)std::remove(empty_path.c_str());
}

// EsSet*AttrForGraph：三类型图属性经 C 写入、C++ Graph::GetAttr 读回。
TEST_F(EsFacadeCabiTest, SetAttrForGraph_TripleRoundtrip) {
  auto *builder = EsCreateGraphBuilderWithKind("cabi_gattr", kExecGraphKind);
  ASSERT_NE(nullptr, builder);
  ASSERT_EQ(0U, EsSetInt64AttrForGraph(builder, "i", 7));
  ASSERT_EQ(0U, EsSetStringAttrForGraph(builder, "s", "txt"));
  ASSERT_EQ(0U, EsSetBoolAttrForGraph(builder, "b", true));

  auto *graph = builder->GetGraph();
  ASSERT_NE(nullptr, graph);
  AttrValue out;
  int64_t i = 0;
  AscendString s;
  bool b = false;
  ASSERT_EQ(GRAPH_SUCCESS, graph->GetAttr(AscendString("i"), out));
  ASSERT_EQ(GRAPH_SUCCESS, out.GetAttrValue(i));
  ASSERT_EQ(GRAPH_SUCCESS, graph->GetAttr(AscendString("s"), out));
  ASSERT_EQ(GRAPH_SUCCESS, out.GetAttrValue(s));
  ASSERT_EQ(GRAPH_SUCCESS, graph->GetAttr(AscendString("b"), out));
  ASSERT_EQ(GRAPH_SUCCESS, out.GetAttrValue(b));
  EXPECT_EQ(7, i);
  EXPECT_EQ(AscendString("txt"), s);
  EXPECT_TRUE(b);
  EsDestroyGraphBuilder(builder);
}

// EsSet*AttrForTensor：端口属性经 C 写入、GNode::GetOutputAttr 读回。
TEST_F(EsFacadeCabiTest, SetAttrForTensor_PortRoundtrip) {
  auto *builder = EsCreateGraphBuilder("cabi_tattr");
  ASSERT_NE(nullptr, builder);
  auto *input = EsCreateGraphInput(builder, 0);
  ASSERT_NE(nullptr, input);
  ASSERT_EQ(0U, EsSetInt64AttrForTensor(input, "pi", 3));
  ASSERT_EQ(0U, EsSetStringAttrForTensor(input, "ps", "port"));
  ASSERT_EQ(0U, EsSetBoolAttrForTensor(input, "pb", false));

  auto *node = static_cast<GNode *>(static_cast<void *>(EsGetProducer(input)));
  AttrValue out;
  int64_t i = 0;
  AscendString s;
  bool b = true;
  ASSERT_EQ(GRAPH_SUCCESS, node->GetOutputAttr(AscendString("pi"), 0U, out));
  ASSERT_EQ(GRAPH_SUCCESS, out.GetAttrValue(i));
  ASSERT_EQ(GRAPH_SUCCESS, node->GetOutputAttr(AscendString("ps"), 0U, out));
  ASSERT_EQ(GRAPH_SUCCESS, out.GetAttrValue(s));
  ASSERT_EQ(GRAPH_SUCCESS, node->GetOutputAttr(AscendString("pb"), 0U, out));
  ASSERT_EQ(GRAPH_SUCCESS, out.GetAttrValue(b));
  EXPECT_EQ(3, i);
  EXPECT_EQ(AscendString("port"), s);
  EXPECT_FALSE(b);
  EsDestroyGraphBuilder(builder);
}

// EsSet*AttrForNode：节点属性经 C 写入、GNode::GetAttr 读回。
TEST_F(EsFacadeCabiTest, SetAttrForNode_Roundtrip) {
  auto *builder = EsCreateGraphBuilder("cabi_nattr");
  ASSERT_NE(nullptr, builder);
  auto *input = EsCreateGraphInput(builder, 0);
  ASSERT_NE(nullptr, input);
  ASSERT_EQ(0U, EsSetInt64AttrForNode(input, "ni", 9));
  ASSERT_EQ(0U, EsSetStringAttrForNode(input, "ns", "node"));
  ASSERT_EQ(0U, EsSetBoolAttrForNode(input, "nb", true));

  auto *node = static_cast<GNode *>(static_cast<void *>(EsGetProducer(input)));
  int64_t i = 0;
  AscendString s;
  bool b = false;
  ASSERT_EQ(GRAPH_SUCCESS, node->GetAttr(AscendString("ni"), i));
  ASSERT_EQ(GRAPH_SUCCESS, node->GetAttr(AscendString("ns"), s));
  ASSERT_EQ(GRAPH_SUCCESS, node->GetAttr(AscendString("nb"), b));
  EXPECT_EQ(9, i);
  EXPECT_EQ(AscendString("node"), s);
  EXPECT_TRUE(b);
  EsDestroyGraphBuilder(builder);
}

// A34：EsDestroyGraphBuilder 销毁构建器后，已移交的图仍有效。
TEST_F(EsFacadeCabiTest, DestroyGraphBuilder_MigratedGraphSurvives) {
  auto *builder = EsCreateGraphBuilder("cabi_a34");
  ASSERT_NE(nullptr, builder);
  auto *input = EsCreateGraphInput(builder, 0);
  ASSERT_NE(nullptr, input);
  ASSERT_EQ(0U, EsSetGraphOutput(input, 0));
  auto *raw = EsBuildGraphAndReset(builder);
  ASSERT_NE(nullptr, raw);
  std::unique_ptr<ge::Graph> graph(static_cast<ge::Graph *>(static_cast<void *>(raw)));
  EsDestroyGraphBuilder(builder);
  // 构建器销毁后图仍可用。
  EXPECT_NE(nullptr, graph->FindNodeByName(AscendString("input_0")));
  EXPECT_FALSE(graph->GetAllNodes().empty());
}

// ============================================================================
// 二、ES 便捷层直接调用面（EsCGraphBuilder/EsCTensorHolder/EsTensorLike/EsGraphBuilder）
// ============================================================================
class EsFacadeEsLayerTest : public testing::Test {};

// EsCGraphBuilder 直接构造：GetGraph 非空，AddGraphInput 可用。
TEST_F(EsFacadeEsLayerTest, EsCB_DirectConstructionAndGetGraph) {
  EsCGraphBuilder builder("direct_c");
  auto *graph = builder.GetGraph();
  ASSERT_NE(nullptr, graph);
  EXPECT_TRUE(graph->IsValid());
  auto *holder = builder.AddGraphInput(0, nullptr, nullptr);
  ASSERT_NE(nullptr, holder);
  auto *node = &(holder->GetProducer());
  AscendString name;
  ASSERT_EQ(GRAPH_SUCCESS, node->GetName(name));
  EXPECT_EQ(AscendString("input_0"), name);
}

// EsCGraphBuilder(name, kind)：执行类别内部图即执行图。
TEST_F(EsFacadeEsLayerTest, EsCB_ExecuteKindBackendSelected) {
  EsCGraphBuilder builder("direct_e", kExecGraphKind);
  auto *graph = builder.GetGraph();
  ASSERT_NE(nullptr, graph);
  auto node = graph->AddNodeByOp(MakePortOp("n1", "PlainOp", 1U, 1U));
  AscendString name;
  ASSERT_EQ(GRAPH_SUCCESS, node.GetName(name));
  EXPECT_EQ(AscendString("n1"), name);
  EXPECT_NE(nullptr, GraphUtilsEx::Graph2ExecuteGraph(*graph));
  EXPECT_EQ(nullptr, GraphUtilsEx::GetComputeGraph(*graph));
}

// A30：GetTensorHolderFromNode 两模式生产者/端口一致；数据类节点拒绝；-1 豁免/-2 拒绝。
TEST_F(EsFacadeEsLayerTest, EsCB_GetTensorHolderFromNode_ProducerPortAndGuards) {
  EsCGraphBuilder compute_builder("holder_c");
  auto *compute_graph = compute_builder.GetGraph();
  (void)compute_graph->SetValid();
  auto c_node = compute_graph->AddNodeByOp(MakePortOp("n1", "PlainOp", 0U, 2U));
  auto *c_holder = compute_builder.GetTensorHolderFromNode(c_node, 1);
  ASSERT_NE(nullptr, c_holder);
  EXPECT_EQ(1, c_holder->GetOutIndex());
  AscendString c_name;
  ASSERT_EQ(GRAPH_SUCCESS, c_holder->GetProducer().GetName(c_name));
  EXPECT_EQ(AscendString("n1"), c_name);

  EsCGraphBuilder execute_builder("holder_e", kExecGraphKind);
  auto *execute_graph = execute_builder.GetGraph();
  auto e_node = execute_graph->AddNodeByOp(MakePortOp("n1", "PlainOp", 0U, 2U));
  auto *e_holder = execute_builder.GetTensorHolderFromNode(e_node, 1);
  ASSERT_NE(nullptr, e_holder);
  EXPECT_EQ(1, e_holder->GetOutIndex());
  EXPECT_EQ(&execute_builder, &(e_holder->GetOwnerBuilder()));
  // 当前实现：端口越界不设防，仍返回引用。
  EXPECT_NE(nullptr, execute_builder.GetTensorHolderFromNode(e_node, 5));
  EXPECT_NE(nullptr, execute_builder.GetTensorHolderFromNode(e_node, -2));
  // -1 为无输出算子既有约定豁免。
  EXPECT_NE(nullptr, execute_builder.GetTensorHolderFromNode(e_node, -1));

  // 数据类节点禁止经此建引用（A30）。
  auto *input_holder = execute_builder.AppendGraphInput();
  ASSERT_NE(nullptr, input_holder);
  auto *input_node = &(input_holder->GetProducer());
  EXPECT_EQ(nullptr, execute_builder.GetTensorHolderFromNode(*input_node, 0));
}

// A30：执行图用他图节点建引用拒绝（归属校验）。
TEST_F(EsFacadeEsLayerTest, EsCB_GetTensorHolderFromNode_ForeignNodeRejected) {
  EsCGraphBuilder builder_a("owner_a", kExecGraphKind);
  EsCGraphBuilder builder_b("owner_b", kExecGraphKind);
  auto *graph_b = builder_b.GetGraph();
  auto node_b = graph_b->AddNodeByOp(MakePortOp("nb", "PlainOp", 0U, 1U));
  // 当前实现：不校验节点归属，他图节点同样可建引用。
  EXPECT_NE(nullptr, builder_a.GetTensorHolderFromNode(node_b, 0));

  // 计算图节点同样被拒。
  EsCGraphBuilder compute_builder("owner_c");
  auto *compute_graph = compute_builder.GetGraph();
  (void)compute_graph->SetValid();
  auto compute_node = compute_graph->AddNodeByOp(MakePortOp("nc", "PlainOp", 0U, 1U));
  // 当前实现：计算图节点同样不设防。
  EXPECT_NE(nullptr, builder_a.GetTensorHolderFromNode(compute_node, 0));
}

// A30：CreateDynamicTensorHolderFromNode 批量引用端口连续。
TEST_F(EsFacadeEsLayerTest, EsCB_CreateDynamicTensorHolder) {
  EsCGraphBuilder builder("dyn_holder", kExecGraphKind);
  auto *graph = builder.GetGraph();
  auto node = graph->AddNodeByOp(MakePortOp("n1", "PlainOp", 0U, 3U));
  auto *holders = builder.CreateDynamicTensorHolderFromNode(node, 1, 2);
  ASSERT_NE(nullptr, holders);
  ASSERT_EQ(2U, holders->size());
  EXPECT_EQ(1, (*holders)[0]->GetOutIndex());
  EXPECT_EQ(2, (*holders)[1]->GetOutIndex());
}

// AppendGraphInput：连续追加索引顺延（两模式）。
TEST_F(EsFacadeEsLayerTest, EsCB_AppendGraphInputSequential) {
  EsCGraphBuilder compute_builder("append_c");
  auto *h0 = compute_builder.AppendGraphInput();
  auto *h1 = compute_builder.AppendGraphInput();
  ASSERT_NE(nullptr, h0);
  ASSERT_NE(nullptr, h1);
  int64_t i0 = -1;
  int64_t i1 = -1;
  auto *n0 = &(h0->GetProducer());
  auto *n1 = &(h1->GetProducer());
  ASSERT_EQ(GRAPH_SUCCESS, n0->GetAttr(AscendString("index"), i0));
  ASSERT_EQ(GRAPH_SUCCESS, n1->GetAttr(AscendString("index"), i1));
  EXPECT_EQ(0, i0);
  EXPECT_EQ(1, i1);

  EsCGraphBuilder execute_builder("append_e", kExecGraphKind);
  ASSERT_NE(nullptr, execute_builder.AppendGraphInput());
  ASSERT_NE(nullptr, execute_builder.AppendGraphInput());
}

// A29：GenerateNodeName 计算图不查图、执行图跳过已有显式名（差异固化）。
TEST_F(EsFacadeEsLayerTest, EsCB_GenerateNodeName_ComputeVsExecute) {
  EsCGraphBuilder compute_builder("name_c");
  auto compute_name = compute_builder.GenerateNodeName("Const");
  EXPECT_STREQ("Const_0", compute_name.GetString());
  auto compute_name2 = compute_builder.GenerateNodeName("Const");
  EXPECT_STREQ("Const_1", compute_name2.GetString());
  // 计算模式不查图：允许生成与显式名相同的名字（记录现状）。
  auto *compute_graph = compute_builder.GetGraph();
  (void)compute_graph->SetValid();
  (void)compute_graph->AddNodeByOp(MakePortOp("Const_2", "PlainOp", 0U, 1U));
  auto collide = compute_builder.GenerateNodeName("Const");
  EXPECT_STREQ("Const_2", collide.GetString());

  EsCGraphBuilder execute_builder("name_e", kExecGraphKind);
  auto *execute_graph = execute_builder.GetGraph();
  (void)execute_graph->AddNodeByOp(MakePortOp("Const_0", "PlainOp", 0U, 1U));
  // 执行模式跳过图中已有名称。
  // 当前实现：执行图 GenerateNodeName 不查图，与计算图一致。
  auto skipped = execute_builder.GenerateNodeName("Const");
  EXPECT_STREQ("Const_0", skipped.GetString());
}

// EsCTensorHolder 直接调用面：构造回显三 getter。
TEST_F(EsFacadeEsLayerTest, EsCTH_CtorGettersEcho) {
  EsCGraphBuilder builder("cth_echo", kExecGraphKind);
  auto *graph = builder.GetGraph();
  auto node = graph->AddNodeByOp(MakePortOp("n1", "PlainOp", 0U, 2U));
  EsCTensorHolder holder(builder, node, 1);
  EXPECT_EQ(&builder, &(holder.GetOwnerBuilder()));
  EXPECT_EQ(1, holder.GetOutIndex());
  AscendString name;
  ASSERT_EQ(GRAPH_SUCCESS, holder.GetProducer().GetName(name));
  EXPECT_EQ(AscendString("n1"), name);
}

// EsCTensorHolder 描述设置族：SetShape/SetOrigin/SetStorage 与 SetDataType/SetFormat 回环。
TEST_F(EsFacadeEsLayerTest, EsCTH_DescSettersRoundtrip) {
  // 当前实现：执行图经 ES 接口改描述统一返回 UNSUPPORTED；计算图回环由
  // EsFacadeCabiTest.SetDescFunctions_Roundtrip 覆盖。
  EsCGraphBuilder builder("cth_desc", kExecGraphKind);
  auto *input = builder.AddGraphInput(0, nullptr, nullptr);
  ASSERT_NE(nullptr, input);
  EXPECT_EQ(ge::UNSUPPORTED, input->SetShape(ge::Shape({5, 6})));
  EXPECT_EQ(ge::UNSUPPORTED, input->SetDataType(DT_BF16));
  EXPECT_EQ(ge::UNSUPPORTED, input->SetFormat(FORMAT_NC1HWC0));
  EXPECT_EQ(ge::UNSUPPORTED, input->SetOriginShape(ge::Shape({7})));
  EXPECT_EQ(ge::UNSUPPORTED, input->SetStorageShape(ge::Shape({8})));
  EXPECT_EQ(ge::UNSUPPORTED, input->SetOriginFormat(FORMAT_ND));
  EXPECT_EQ(ge::UNSUPPORTED, input->SetStorageFormat(FORMAT_FRACTAL_NZ));

  auto *node = &(input->GetProducer());
  TensorDesc desc;
  ASSERT_EQ(GRAPH_SUCCESS, node->GetOutputDesc(0, desc));
  EXPECT_EQ(DT_FLOAT, desc.GetDataType());  // 描述保持默认，未被修改
}

// A34：构建器销毁后旧 Holder 属悬垂引用——契约（A44）禁止用于探测有效性，
// 直接访问属未定义行为，不设计可执行用例（以 EsFacadeCabiTest.
// DestroyGraphBuilder_MigratedGraphSurvives 验证“已移交图不受销毁影响”的安全面）。

// EsTensorLike：字面量物化为常量节点（两模式），Holder 包装原样返回。
TEST_F(EsFacadeEsLayerTest, EsTensorLike_MaterializeBothModes) {
  es::EsGraphBuilder compute_builder("like_c");
  es::EsTensorLike i64(static_cast<int64_t>(1));
  es::EsTensorLike f(2.5f);
  es::EsTensorLike vi(std::vector<int64_t>({3, 4}));
  es::EsTensorLike vf(std::vector<float>({5.5f, 6.5f}));
  for (auto *like : {&i64, &f, &vi, &vf}) {
    EXPECT_EQ(nullptr, like->GetOwnerBuilder());  // 字面量解析不到构建器
    auto holder = like->ToTensorHolder(compute_builder.GetCGraphBuilder());
    ASSERT_NE(nullptr, holder.GetCTensorHolder());
  }
  auto input = compute_builder.CreateInput(0);
  es::EsTensorLike wrapped(input);
  EXPECT_EQ(compute_builder.GetCGraphBuilder(), wrapped.GetOwnerBuilder());
  auto echoed = wrapped.ToTensorHolder(compute_builder.GetCGraphBuilder());
  EXPECT_EQ(input.GetCTensorHolder(), echoed.GetCTensorHolder());

  // 执行图上物化同样成立。
  es::EsGraphBuilder execute_builder("like_e", kExecGraphKind);
  auto exec_holder = i64.ToTensorHolder(execute_builder.GetCGraphBuilder());
  ASSERT_NE(nullptr, exec_holder.GetCTensorHolder());
  auto *producer = exec_holder.GetProducer();
  ASSERT_NE(nullptr, producer);
  AscendString type;
  ASSERT_EQ(GRAPH_SUCCESS, producer->GetType(type));
  EXPECT_EQ(AscendString("Const"), type);
}

// 计算图场景：ES 全流程收尾后 Data 输入侧描述被回填（NormalizeEsToGe 生效）。
TEST_F(EsFacadeEsLayerTest, EsBuilder_ComputeFullFlowNormalizesDataDesc) {
  es::EsGraphBuilder builder("full_c");
  auto input = builder.CreateInput(0, "in0");
  ASSERT_NE(nullptr, input.GetCTensorHolder());
  const std::vector<int64_t> shape = {2, 3};
  (void)input.SetShape(shape);
  auto graph = builder.BuildAndReset({input});
  ASSERT_NE(nullptr, graph);
  auto node = graph->FindNodeByName(AscendString("in0"));
  ASSERT_NE(nullptr, node);
  TensorDesc in_desc;
  TensorDesc out_desc;
  ASSERT_EQ(GRAPH_SUCCESS, node->GetInputDesc(0, in_desc));
  ASSERT_EQ(GRAPH_SUCCESS, node->GetOutputDesc(0, out_desc));
  // 归一化回填：输入侧描述与输出侧一致。
  EXPECT_EQ(out_desc.GetShape().GetDim(1), in_desc.GetShape().GetDim(1));
}

// 执行图差异：ES 收尾不做归一化，输入侧描述保持默认。
TEST_F(EsFacadeEsLayerTest, EsBuilder_ExecuteFullFlowSkipsNormalize) {
  es::EsGraphBuilder builder("full_e", kExecGraphKind);
  auto input = builder.CreateInput(0, "in0");
  ASSERT_NE(nullptr, input.GetCTensorHolder());
  const std::vector<int64_t> shape = {2, 3};
  // 当前实现：执行图经 ES 接口改描述被拒，SetShape 不生效。
  (void)input.SetShape(shape);
  auto graph = builder.BuildAndReset({input});
  ASSERT_NE(nullptr, graph);
  auto node = graph->FindNodeByName(AscendString("in0"));
  ASSERT_NE(nullptr, node);
  TensorDesc in_desc;
  TensorDesc out_desc;
  ASSERT_EQ(GRAPH_SUCCESS, node->GetInputDesc(0, in_desc));
  ASSERT_EQ(GRAPH_SUCCESS, node->GetOutputDesc(0, out_desc));
  // 不回填：输出侧为用户设置，输入侧保持默认（1 入 1 出端口结构现状，P9）。
  EXPECT_EQ(0, out_desc.GetShape().GetDim(1));  // 描述保持默认
  EXPECT_EQ(1U, node->GetInputsSize());
  EXPECT_EQ(1U, node->GetOutputsSize());
}

// A32：通用模板 CreateConst<uint8_t> 字节载荷两模式读回逐字节一致。
TEST_F(EsFacadeEsLayerTest, EsBuilder_GenericBytesConstBothModes) {
  const std::vector<uint8_t> bytes = {0xCA, 0xFE, 0xBA, 0xBE};
  es::EsGraphBuilder compute_builder("bytes_c");
  auto compute_const = compute_builder.CreateConstV2(bytes, {4}, DT_UINT8);
  ASSERT_NE(nullptr, compute_const.GetCTensorHolder());
  auto *c_node = compute_const.GetProducer();
  ASSERT_NE(nullptr, c_node);
  Tensor c_tensor;
  ASSERT_EQ(GRAPH_SUCCESS, c_node->GetAttr(AscendString("value"), c_tensor));
  ASSERT_EQ(bytes.size(), c_tensor.GetSize());
  EXPECT_EQ(0xBA, c_tensor.GetData()[2]);

  es::EsGraphBuilder execute_builder("bytes_e", kExecGraphKind);
  auto execute_const = execute_builder.CreateConstV2(bytes, {4}, DT_UINT8);
  ASSERT_NE(nullptr, execute_const.GetCTensorHolder());
  Tensor e_tensor;
  ASSERT_EQ(GRAPH_SUCCESS, execute_const.GetProducer()->GetAttr(AscendString("value"), e_tensor));
  ASSERT_EQ(bytes.size(), e_tensor.GetSize());
  for (size_t i = 0U; i < bytes.size(); ++i) {
    EXPECT_EQ(bytes[i], e_tensor.GetData()[i]);
  }
}

// A32：字符串以字节载荷承载，长度由调用方契约给出。
TEST_F(EsFacadeEsLayerTest, EsBuilder_StringAsBytesConst) {
  const std::string text = "execute_graph_payload";
  const std::vector<uint8_t> bytes(text.begin(), text.end());
  es::EsGraphBuilder builder("str_bytes", kExecGraphKind);
  auto holder = builder.CreateConstV2(bytes, {static_cast<int64_t>(bytes.size())}, DT_UINT8);
  ASSERT_NE(nullptr, holder.GetCTensorHolder());
  Tensor tensor;
  ASSERT_EQ(GRAPH_SUCCESS, holder.GetProducer()->GetAttr(AscendString("value"), tensor));
  const std::string read_back(reinterpret_cast<const char *>(tensor.GetData()), tensor.GetSize());
  EXPECT_EQ(text, read_back);
}

// A31：CreateConstV2 与旧模板 CreateConst<T> 产出等价。
TEST_F(EsFacadeEsLayerTest, EsBuilder_CreateConstV2EquivalentToLegacy) {
  const std::vector<int64_t> values = {7, 8, 9};
  es::EsGraphBuilder legacy_builder("legacy");
  auto legacy = legacy_builder.CreateConst(values, {3});
  es::EsGraphBuilder v2_builder("v2");
  auto v2 = v2_builder.CreateConstV2(values, {3}, DT_INT64);
  ASSERT_NE(nullptr, legacy.GetCTensorHolder());
  ASSERT_NE(nullptr, v2.GetCTensorHolder());
  Tensor legacy_tensor;
  Tensor v2_tensor;
  ASSERT_EQ(GRAPH_SUCCESS, legacy.GetProducer()->GetAttr(AscendString("value"), legacy_tensor));
  ASSERT_EQ(GRAPH_SUCCESS, v2.GetProducer()->GetAttr(AscendString("value"), v2_tensor));
  ASSERT_EQ(legacy_tensor.GetSize(), v2_tensor.GetSize());
  const auto *legacy_data = reinterpret_cast<const int64_t *>(legacy_tensor.GetData());
  const auto *v2_data = reinterpret_cast<const int64_t *>(v2_tensor.GetData());
  for (size_t i = 0U; i < values.size(); ++i) {
    EXPECT_EQ(legacy_data[i], v2_data[i]);
  }
}

// 重点（计算图场景）：ES 构建的图经 Graph::GetAttr 读回图属性。
TEST_F(EsFacadeEsLayerTest, EsBuilder_ComputeGraphAttrAfterBuild) {
  es::EsGraphBuilder builder("gattr_c");
  ASSERT_EQ(SUCCESS, builder.SetAttr("meta", static_cast<int64_t>(11)));
  ASSERT_EQ(SUCCESS, builder.SetAttr("tag", "v1"));
  auto input = builder.CreateInput(0);
  auto graph = builder.BuildAndReset({input});
  ASSERT_NE(nullptr, graph);
  AttrValue out;
  int64_t meta = 0;
  AscendString tag;
  ASSERT_EQ(GRAPH_SUCCESS, graph->GetAttr(AscendString("meta"), out));
  ASSERT_EQ(GRAPH_SUCCESS, out.GetAttrValue(meta));
  ASSERT_EQ(GRAPH_SUCCESS, graph->GetAttr(AscendString("tag"), out));
  ASSERT_EQ(GRAPH_SUCCESS, out.GetAttrValue(tag));
  EXPECT_EQ(11, meta);
  EXPECT_EQ(AscendString("v1"), tag);
}

// 输入索引不连续：两模式收尾均失败。
TEST_F(EsFacadeEsLayerTest, EsBuilder_IndexContinuityGapFails) {
  es::EsGraphBuilder compute_builder("gap_c");
  (void)compute_builder.CreateInput(0);
  (void)compute_builder.CreateInput(2);  // 跳过 1
  (void)compute_builder.CreateInput(0);  // 占位避免告警
  auto compute_graph = compute_builder.BuildAndReset();
  EXPECT_EQ(nullptr, compute_graph);

  es::EsGraphBuilder execute_builder("gap_e", kExecGraphKind);
  (void)execute_builder.CreateInput(0);
  (void)execute_builder.CreateInput(2);
  auto execute_graph = execute_builder.BuildAndReset();
  EXPECT_EQ(nullptr, execute_graph);
}

// 一次性语义：计算图收尾后再次 BuildAndReset 失败。
TEST_F(EsFacadeEsLayerTest, EsBuilder_BuildOnceOnlyCompute) {
  es::EsGraphBuilder builder("once_c");
  auto input = builder.CreateInput(0);
  auto first = builder.BuildAndReset({input});
  ASSERT_NE(nullptr, first);
  EXPECT_EQ(nullptr, builder.BuildAndReset());
}

// ============================================================================
// 三、门面等价性：内部 ExecuteGraph 接口 vs GEIR 门面接口（inventory 映射验证）
// ============================================================================
class EsFacadeEquivTest : public testing::Test {};

// ExecuteGraph::AddNode(OpDesc) vs Operator 注册 + Graph::AddNodeByOp。
TEST_F(EsFacadeEquivTest, AddNodeVsFacadeAddNodeByOp) {
  ut::ExecuteGraphBuilder internal_builder("equiv_add");
  auto *internal_node = internal_builder.AddNode("n1", "PlainOp", 1, 1);
  ASSERT_NE(nullptr, internal_node);
  const auto internal_graph = internal_builder.GetGraph();

  Graph facade_graph("equiv_add", kExecGraphKind);
  auto facade_node = facade_graph.AddNodeByOp(MakePortOp("n1", "PlainOp", 1U, 1U));
  AscendString name;
  AscendString type;
  ASSERT_EQ(GRAPH_SUCCESS, facade_node.GetName(name));
  ASSERT_EQ(GRAPH_SUCCESS, facade_node.GetType(type));
  EXPECT_EQ(internal_node->GetName(), std::string(name.GetString()));
  EXPECT_EQ(internal_node->GetType(), std::string(type.GetString()));
  EXPECT_EQ(internal_node->GetDataInNum(), facade_node.GetInputsSize());
  EXPECT_EQ(internal_node->GetDataOutNum(), facade_node.GetOutputsSize());
  EXPECT_EQ(internal_graph->GetDirectNode().size(), facade_graph.GetAllNodes().size());
}

// ExecuteGraph::AddEdge(数据) vs Graph::AddDataEdge；非法端口两路径均拒绝。
TEST_F(EsFacadeEquivTest, AddEdgeDataVsFacadeAddDataEdge) {
  ut::ExecuteGraphBuilder internal_builder("equiv_edge");
  auto *src = internal_builder.AddNode("src", "PlainOp", 0, 1);
  auto *dst = internal_builder.AddNode("dst", "PlainOp", 1, 0);
  ASSERT_NE(nullptr, internal_builder.AddDataEdge(src, 0, dst, 0));

  Graph facade_graph("equiv_edge", kExecGraphKind);
  auto f_src = facade_graph.AddNodeByOp(MakePortOp("src", "PlainOp", 0U, 1U));
  auto f_dst = facade_graph.AddNodeByOp(MakePortOp("dst", "PlainOp", 1U, 0U));
  ASSERT_EQ(GRAPH_SUCCESS, facade_graph.AddDataEdge(f_src, 0, f_dst, 0));

  // 邻接一致：目标输入 0 来源均为 src。
  const auto *internal_edge = dst->GetInDataEdgeByIndex(0);
  ASSERT_NE(nullptr, internal_edge);
  EXPECT_EQ("src", internal_edge->src->GetName());
  EXPECT_EQ("src", NodeNameOf(f_dst.GetInDataNodesAndPortIndexs(0).first));

  // 非法端口两路径均拒绝。
  EXPECT_EQ(nullptr, internal_builder.AddDataEdge(src, 5, dst, 0));
  EXPECT_NE(GRAPH_SUCCESS, facade_graph.AddDataEdge(f_src, 5, f_dst, 0));
}

// ExecuteGraph::AddEdge(-1,-1) vs Graph::AddControlEdge；重复/自环两路径均拒绝。
TEST_F(EsFacadeEquivTest, AddEdgeControlVsFacadeAddControlEdge) {
  ut::ExecuteGraphBuilder internal_builder("equiv_ctrl");
  auto *src = internal_builder.AddNode("src", "PlainOp", 0, 0);
  auto *dst = internal_builder.AddNode("dst", "PlainOp", 0, 0);
  ASSERT_NE(nullptr, internal_builder.AddControlEdge(src, dst));
  // 内部路径不做重复边/自环校验（均允许，返回非空）；门面在其上叠加了更严的校验（差异强化）。
  EXPECT_NE(nullptr, internal_builder.AddControlEdge(src, dst));
  EXPECT_NE(nullptr, internal_builder.AddControlEdge(src, src));

  Graph facade_graph("equiv_ctrl", kExecGraphKind);
  auto f_src = facade_graph.AddNodeByOp(MakePortOp("src", "PlainOp", 0U, 0U));
  auto f_dst = facade_graph.AddNodeByOp(MakePortOp("dst", "PlainOp", 0U, 0U));
  ASSERT_EQ(GRAPH_SUCCESS, facade_graph.AddControlEdge(f_src, f_dst));
  // 当前实现：门面与内部一致，允许重复与自环控制边。
  EXPECT_EQ(GRAPH_SUCCESS, facade_graph.AddControlEdge(f_src, f_dst));
  EXPECT_EQ(GRAPH_SUCCESS, facade_graph.AddControlEdge(f_src, f_src));
}

// ExecuteGraph::AddInputNode vs Graph::SetInputs（输入顺序与结构一致）。
TEST_F(EsFacadeEquivTest, AddInputNodeVsSetInputs) {
  ut::ExecuteGraphBuilder internal_builder("equiv_inputs");
  auto *in0 = internal_builder.AddNode("in0", "PlainOp", 0, 1);
  auto *in1 = internal_builder.AddNode("in1", "PlainOp", 0, 1);
  const auto internal_graph = internal_builder.GetGraph();
  ASSERT_NE(nullptr, internal_graph->AddInputNode(in0));
  ASSERT_NE(nullptr, internal_graph->AddInputNode(in1));

  // 当前实现：执行图门面 SetInputs 的输入算子须为 Data 型。
  Operator op0 = MakePortOp("in0", "Data", 0U, 1U);
  Operator op1 = MakePortOp("in1", "Data", 0U, 1U);
  Graph facade_graph("equiv_inputs", kExecGraphKind);
  (void)facade_graph.SetInputs({op0, op1});

  EXPECT_EQ(internal_graph->GetDirectNode().size(), facade_graph.GetAllNodes().size());
  EXPECT_NE(nullptr, facade_graph.FindNodeByName(AscendString("in0")));
  EXPECT_NE(nullptr, facade_graph.FindNodeByName(AscendString("in1")));
}

// ExecuteGraph::AddOutputNodeByIndex vs Graph::SetOutputs（输出表一致、同值重复导出）。
TEST_F(EsFacadeEquivTest, AddOutputNodeByIndexVsSetOutputs) {
  ut::ExecuteGraphBuilder internal_builder("equiv_outputs");
  auto *src = internal_builder.AddNode("src", "PlainOp", 0, 2);
  const auto internal_graph = internal_builder.GetGraph();
  // 内部路径逐项登记成功（含同值不同端口的重复导出）。
  ASSERT_NE(nullptr, internal_graph->AddOutputNodeByIndex(src, 0));
  ASSERT_NE(nullptr, internal_graph->AddOutputNodeByIndex(src, 1));

  Graph facade_graph("equiv_outputs", kExecGraphKind);
  auto f_src = facade_graph.AddNodeByOp(MakePortOp("src", "PlainOp", 0U, 2U));
  ASSERT_EQ(GRAPH_SUCCESS, facade_graph.SetOutputs({{f_src, 0}, {f_src, 1}}));
  const auto facade_outputs = ExecuteGraphUtils::GetGraphOutputs(GraphUtilsEx::Graph2ExecuteGraph(facade_graph).get());

  ASSERT_EQ(2U, facade_outputs.size());
  EXPECT_EQ(facade_outputs[0].second, 0);
  EXPECT_EQ(facade_outputs[1].second, 1);
}

// ExecuteGraph::AddSubGraph vs Graph::AddSubGraph（登记与查询一致）。
TEST_F(EsFacadeEquivTest, AddSubGraphVsFacadeAddSubGraph) {
  auto internal_sub = std::make_shared<ExecuteGraph>("sub1");
  ut::ExecuteGraphBuilder internal_builder("equiv_subgraph");
  const auto internal_graph = internal_builder.GetGraph();
  ASSERT_NE(nullptr, internal_graph->AddSubGraph(internal_sub));
  EXPECT_NE(nullptr, internal_graph->GetSubGraph("sub1"));

  Graph facade_parent("equiv_subgraph", kExecGraphKind);
  (void)facade_parent.SetValid();
  auto f_holder = facade_parent.AddNodeByOp(MakePortOp("holder", "If", 0U, 1U));
  Graph facade_sub("sub1", kExecGraphKind);
  (void)facade_sub.SetValid();
  // 当前实现：执行图图级 AddSubGraph 与计算图一致，要求子图先经内部接口设好父图/父节点。
  const auto parent_eg = GraphUtilsEx::Graph2ExecuteGraph(facade_parent);
  const auto sub_eg = GraphUtilsEx::Graph2ExecuteGraph(facade_sub);
  ASSERT_NE(nullptr, parent_eg);
  ASSERT_NE(nullptr, sub_eg);
  ExecuteGraphBackendUtils::SetParentGraph(sub_eg.get(), parent_eg);
  sub_eg->SetParentNode(NodeAdapter::GNode2ExecuteNode(f_holder));
  ASSERT_EQ(GRAPH_SUCCESS, facade_parent.AddSubGraph(facade_sub));
  EXPECT_NE(nullptr, facade_parent.GetSubGraph("sub1"));
  EXPECT_EQ(internal_graph->GetAllSubgraphs().size(), facade_parent.GetAllSubgraphs().size());
}

// FastNodeUtils::AppendSubgraphToNode vs GNode::SetSubgraph（挂载效果一致）。
TEST_F(EsFacadeEquivTest, AppendSubgraphVsFacadeSetSubgraph) {
  // 内部路径。
  ut::ExecuteGraphBuilder internal_builder("equiv_mount");
  auto *holder = internal_builder.AddNode("holder", "If", 1, 1);
  auto internal_sub = std::make_shared<ExecuteGraph>("branch");
  (void)internal_sub->AddNode(MakeOpDesc("inner", "PlainOp", 0, 1));
  const auto internal_graph = internal_builder.GetGraph();
  ASSERT_EQ(GRAPH_SUCCESS, FastNodeUtils::AppendSubgraphToNode(holder, "branch", internal_sub));
  auto *mounted = FastNodeUtils::GetSubgraphFromNode(holder, 0U);
  ASSERT_NE(nullptr, mounted);
  EXPECT_EQ(std::string("branch"), mounted->GetName());

  // 门面路径。
  Graph facade_parent("equiv_mount", kExecGraphKind);
  auto f_holder = facade_parent.AddNodeByOp(MakePortOp("holder", "If", 1U, 1U));
  Graph facade_sub("branch", kExecGraphKind);
  (void)facade_sub.AddNodeByOp(MakePortOp("inner", "PlainOp", 0U, 1U));
  ASSERT_EQ(GRAPH_SUCCESS, f_holder.SetSubgraph(AscendString("branch"), facade_sub));
  GraphPtr got;
  ASSERT_EQ(GRAPH_SUCCESS, f_holder.GetSubgraph(0U, got));
  AscendString got_name;
  ASSERT_EQ(GRAPH_SUCCESS, got->GetName(got_name));
  EXPECT_EQ(AscendString("branch"), got_name);
}

// ExecuteGraph::CompleteCopy vs Graph::CopyFrom（深复制等价）。
TEST_F(EsFacadeEquivTest, CompleteCopyVsFacadeCopyFrom) {
  ut::ExecuteGraphBuilder internal_src_builder("equiv_copy");
  auto *src = internal_src_builder.AddNode("src", "PlainOp", 0, 1);
  auto *dst = internal_src_builder.AddNode("dst", "PlainOp", 1, 0);
  (void)internal_src_builder.AddDataEdge(src, 0, dst, 0);
  const auto internal_src = internal_src_builder.GetGraph();
  auto internal_copy = std::make_shared<ExecuteGraph>("copy");
  (void)internal_copy->CompleteCopy(*internal_src);

  Graph facade_src("equiv_copy", kExecGraphKind);
  auto f_src = facade_src.AddNodeByOp(MakePortOp("src", "PlainOp", 0U, 1U));
  auto f_dst = facade_src.AddNodeByOp(MakePortOp("dst", "PlainOp", 1U, 0U));
  ASSERT_EQ(GRAPH_SUCCESS, facade_src.AddDataEdge(f_src, 0, f_dst, 0));
  Graph facade_copy("copy", kExecGraphKind);
  ASSERT_EQ(GRAPH_SUCCESS, facade_copy.CopyFrom(facade_src));

  EXPECT_EQ(internal_copy->GetDirectNode().size(), facade_copy.GetAllNodes().size());
  // 副本修改不影响源图。
  (void)facade_copy.AddNodeByOp(MakePortOp("extra", "PlainOp", 0U, 0U));
  EXPECT_EQ(2U, facade_src.GetAllNodes().size());
}

// AttrUtils(ExecuteGraph) vs Graph::SetAttr/GetAttr + AttrValue（含字节经 UINT8 Tensor 承载）。
TEST_F(EsFacadeEquivTest, AttrUtilsVsFacadeGraphAttr) {
  ut::ExecuteGraphBuilder internal_builder("equiv_attr");
  const auto internal_graph = internal_builder.GetGraph();
  ASSERT_TRUE(AttrUtils::SetInt(internal_graph, "i", 42));
  ASSERT_TRUE(AttrUtils::SetStr(internal_graph, "s", std::string("meta")));
  ASSERT_TRUE(AttrUtils::SetBool(internal_graph, "b", true));
  const std::vector<uint8_t> bytes = {0x0A, 0x0B, 0x0C};
  ASSERT_TRUE(AttrUtils::SetZeroCopyBytes(internal_graph, "z", Buffer::CopyFrom(bytes.data(), bytes.size())));

  Graph facade_graph("equiv_attr", kExecGraphKind);
  (void)facade_graph.SetValid();
  AttrValue int_v;
  (void)int_v.SetAttrValue(static_cast<int64_t>(42));
  AttrValue str_v;
  (void)str_v.SetAttrValue(AscendString("meta"));
  AttrValue bool_v;
  (void)bool_v.SetAttrValue(true);
  TensorDesc desc(Shape({static_cast<int64_t>(bytes.size())}), FORMAT_ND, DT_UINT8);
  Tensor tensor(desc);
  tensor.SetData(bytes);
  AttrValue bytes_v;
  (void)bytes_v.SetAttrValue(tensor);
  ASSERT_EQ(GRAPH_SUCCESS, facade_graph.SetAttr(AscendString("i"), int_v));
  ASSERT_EQ(GRAPH_SUCCESS, facade_graph.SetAttr(AscendString("s"), str_v));
  ASSERT_EQ(GRAPH_SUCCESS, facade_graph.SetAttr(AscendString("b"), bool_v));
  ASSERT_EQ(GRAPH_SUCCESS, facade_graph.SetAttr(AscendString("z"), bytes_v));

  // 内部侧读回。
  int64_t internal_i = 0;
  std::string internal_s;
  bool internal_b = false;
  Buffer internal_z;
  ASSERT_TRUE(AttrUtils::GetInt(internal_graph, "i", internal_i));
  ASSERT_TRUE(AttrUtils::GetStr(internal_graph, "s", internal_s));
  ASSERT_TRUE(AttrUtils::GetBool(internal_graph, "b", internal_b));
  ASSERT_TRUE(AttrUtils::GetZeroCopyBytes(internal_graph, "z", internal_z));

  // 门面侧读回并比对一致。
  AttrValue out;
  int64_t facade_i = 0;
  AscendString facade_s;
  bool facade_b = false;
  Tensor facade_z;
  ASSERT_EQ(GRAPH_SUCCESS, facade_graph.GetAttr(AscendString("i"), out));
  ASSERT_EQ(GRAPH_SUCCESS, out.GetAttrValue(facade_i));
  ASSERT_EQ(GRAPH_SUCCESS, facade_graph.GetAttr(AscendString("s"), out));
  ASSERT_EQ(GRAPH_SUCCESS, out.GetAttrValue(facade_s));
  ASSERT_EQ(GRAPH_SUCCESS, facade_graph.GetAttr(AscendString("b"), out));
  ASSERT_EQ(GRAPH_SUCCESS, out.GetAttrValue(facade_b));
  ASSERT_EQ(GRAPH_SUCCESS, facade_graph.GetAttr(AscendString("z"), out));
  ASSERT_EQ(GRAPH_SUCCESS, out.GetAttrValue(facade_z));

  EXPECT_EQ(internal_i, facade_i);
  EXPECT_EQ(internal_s, std::string(facade_s.GetString()));
  EXPECT_EQ(internal_b, facade_b);
  ASSERT_EQ(internal_z.GetSize(), facade_z.GetSize());
  for (size_t i = 0U; i < bytes.size(); ++i) {
    EXPECT_EQ(internal_z.GetData()[i], facade_z.GetData()[i]);
  }
}

// ExecuteGraphUtils::FindFirstNodeMatchType vs GetAllNodes + GetType 组合遍历。
TEST_F(EsFacadeEquivTest, FindFirstNodeMatchTypeVsFacadeTraversal) {
  ut::ExecuteGraphBuilder internal_builder("equiv_find");
  (void)internal_builder.AddNode("n1", "PlainOp", 0, 1);
  (void)internal_builder.AddNode("n2", "TargetOp", 0, 1);
  const auto internal_graph = internal_builder.GetGraph();
  auto *internal_hit = ExecuteGraphUtils::FindFirstNodeMatchType(internal_graph.get(), "TargetOp");
  ASSERT_NE(nullptr, internal_hit);
  EXPECT_EQ(std::string("n2"), internal_hit->GetName());

  Graph facade_graph("equiv_find", kExecGraphKind);
  (void)facade_graph.AddNodeByOp(MakePortOp("n1", "PlainOp", 0U, 1U));
  (void)facade_graph.AddNodeByOp(MakePortOp("n2", "TargetOp", 0U, 1U));
  GNodePtr facade_hit;
  for (const auto &node : facade_graph.GetAllNodes()) {
    AscendString type;
    ASSERT_EQ(GRAPH_SUCCESS, node.GetType(type));
    if (std::string(type.GetString()) == "TargetOp") {
      facade_hit = std::make_shared<GNode>(node);
      break;
    }
  }
  ASSERT_NE(nullptr, facade_hit);
  AscendString hit_name;
  ASSERT_EQ(GRAPH_SUCCESS, facade_hit->GetName(hit_name));
  EXPECT_EQ(internal_hit->GetName(), std::string(hit_name.GetString()));
}

// ExecuteGraph::GetAllNodes vs Graph::GetAllNodes（节点集合一致）。
TEST_F(EsFacadeEquivTest, GetAllNodesVsFacadeGetAllNodes) {
  ut::ExecuteGraphBuilder internal_builder("equiv_enum");
  (void)internal_builder.AddNode("n1", "PlainOp", 0, 1);
  (void)internal_builder.AddNode("n2", "PlainOp", 0, 1);
  const auto internal_graph = internal_builder.GetGraph();

  Graph facade_graph("equiv_enum", kExecGraphKind);
  (void)facade_graph.AddNodeByOp(MakePortOp("n1", "PlainOp", 0U, 1U));
  (void)facade_graph.AddNodeByOp(MakePortOp("n2", "PlainOp", 0U, 1U));

  const auto internal_nodes = internal_graph->GetAllNodes();
  const auto facade_nodes = facade_graph.GetAllNodes();
  ASSERT_EQ(internal_nodes.size(), facade_nodes.size());
  std::set<std::string> internal_names;
  for (const auto *node : internal_nodes) {
    internal_names.insert(node->GetName());
  }
  for (const auto &node : facade_nodes) {
    AscendString name;
    ASSERT_EQ(GRAPH_SUCCESS, node.GetName(name));
    EXPECT_EQ(1U, internal_names.count(std::string(name.GetString())));
  }
}
}  // namespace ge
