/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include <gtest/gtest.h>
#include <memory>
#include <fstream>
#include <limits>

#include "macro_utils/dt_public_scope.h"
#include "common/file_constant_utils/file_constant_utils.h"
#include "macro_utils/dt_public_unscope.h"

#include "common/helper/file_saver.h"
#include "graph/passes/graph_builder_utils.h"
#include "graph/ge_local_context.h"
#include "ge_graph_dsl/graph_dsl.h"
#include "graph/load/model_manager/davinci_model.h"
#include "graph/manager/graph_external_weight_manager.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/constant_utils.h"
#include "graph/ge_context.h"
#include "depends/mmpa/src/mmpa_stub.h"

namespace ge {
namespace {

class MockMmpa : public MmpaStubApiGe {
 public:
  INT32 Open2(const CHAR *path_name, INT32 flags, MODE mode) override {
    return -1;
  }
};
class MockMmpaForFlockFailed : public MmpaStubApiGe {
 public:
  INT32 Open2(const CHAR *path_name, INT32 flags, MODE mode) override {
    return INT32_MAX;
  }
};
}  // namespace
static ge::OpDescPtr CreateOpDesc(string name = "", string type = "") {
  auto op_desc = std::make_shared<ge::OpDesc>(name, type);
  op_desc->SetStreamId(0);
  op_desc->SetId(0);

  op_desc->SetWorkspace({});
  op_desc->SetWorkspaceBytes({});
  op_desc->SetInputOffset({100, 200});
  op_desc->SetOutputOffset({100, 200});
  return op_desc;
}

namespace fileconstant {
class UtestFileConstantUtilTransfer : public testing::Test {
 protected:
  void SetUp() {
    ExternalWeightManagerPool::Instance().Destroy();
  }
  void TearDown() {
    ExternalWeightManagerPool::Instance().Destroy();
  }
};

TEST_F(UtestFileConstantUtilTransfer, GetFilePathFromOptionOK) {
  std::map<std::string, std::string> options;
  options["ge.exec.value_bins"] =
      "{\"value_bins\":[{\"value_bin_id\":\"vector_search_buchet_value_bin\", \"value_bin_file\":\"hello.bin\"}]}";
  ge::GetThreadLocalContext().SetGraphOption(options);
  std::map<std::string, std::string> file_id_and_path_map;
  EXPECT_EQ(FileConstantUtils::GetFileIdToPathMapFromOption(file_id_and_path_map), SUCCESS);
  EXPECT_EQ(file_id_and_path_map.size(), 1);
}

TEST_F(UtestFileConstantUtilTransfer, GetFilePathFromOptionFailed) {
  std::map<std::string, std::string> options;
  options["ge.exec.value_bins"] = "{\"value_bins\":";
  ge::GetThreadLocalContext().SetGraphOption(options);
  std::map<std::string, std::string> file_id_and_path_map;
  EXPECT_NE(FileConstantUtils::GetFileIdToPathMapFromOption(file_id_and_path_map), SUCCESS);
}

TEST_F(UtestFileConstantUtilTransfer, CopyOneWeightFromFileOK) {
  GetContext().SetSessionId(0U);
  std::unique_ptr<char[]> buf(new char[2048]);
  string file_name = "tmp_weight_pid/no_find_file";
  size_t file_const_size = 100;
  size_t left_size = 0;
  Status ret = FileConstantUtils::CopyOneWeightFromFile((void *)buf.get(), file_name, 0U, file_const_size, left_size);
  EXPECT_EQ(ret, PARAM_INVALID);
  left_size = file_const_size;
  ret = FileConstantUtils::CopyOneWeightFromFile((void *)buf.get(), "", 0U, file_const_size, left_size);
  EXPECT_EQ(ret, FAILED);
  ret = FileConstantUtils::CopyOneWeightFromFile((void *)buf.get(), file_name, 0U, file_const_size, left_size);
  EXPECT_EQ(ret, GRAPH_FAILED);

  std::unique_ptr<float[]> float_buf(new float[file_const_size / sizeof(float)]);
  file_name = "tmp_weight_pid/test_copy_one_weight.bin";
  std::ofstream out1(file_name, std::ios::binary);
  if (!out1.is_open()) {
    return;
  }
  out1.write((char *)float_buf.get(), file_const_size);
  out1.close();

  ret = FileConstantUtils::CopyOneWeightFromFile((void *)buf.get(), file_name, 0U, file_const_size, left_size);
  EXPECT_EQ(ret, SUCCESS);

  (void)remove("tmp_weight_pid/test_copy_one_weight.bin");
}

TEST_F(UtestFileConstantUtilTransfer, CopyOneWeightFromFileTruncatedFile) {
  const std::string file_name = "test_truncated_weight.bin";
  {
    std::ofstream out(file_name, std::ios::binary);
    ASSERT_TRUE(out.is_open());
    const std::string data(64U, 'A');
    out.write(data.data(), static_cast<std::streamsize>(data.size()));
  }
  std::unique_ptr<char[]> buf(new char[256]);
  size_t left_size = 256U;
  // 文件仅 64 字节而声明读 128 字节, 必须失败而不是静默少读
  EXPECT_NE(FileConstantUtils::CopyOneWeightFromFile(buf.get(), file_name, 0U, 128U, left_size), SUCCESS);
  EXPECT_EQ(left_size, 256U);
  // offset 落在 EOF 之后
  left_size = 256U;
  EXPECT_NE(FileConstantUtils::CopyOneWeightFromFile(buf.get(), file_name, 128U, 16U, left_size), SUCCESS);
  EXPECT_EQ(left_size, 256U);
  // int64 负 offset 转 size_t 后的超大值, 必须快速失败而不是死循环
  left_size = 256U;
  EXPECT_NE(FileConstantUtils::CopyOneWeightFromFile(buf.get(), file_name, static_cast<size_t>(-8), 16U, left_size),
            SUCCESS);
  EXPECT_EQ(left_size, 256U);
  // 合法范围内仍成功
  left_size = 256U;
  EXPECT_EQ(FileConstantUtils::CopyOneWeightFromFile(buf.get(), file_name, 16U, 32U, left_size), SUCCESS);
  EXPECT_EQ(left_size, 256U - 32U);
  (void)remove(file_name.c_str());
}

TEST_F(UtestFileConstantUtilTransfer, CopyOneWeightFromFileShortLengthClearTail) {
  const std::string file_name = "test_short_length_weight.bin";
  {
    std::ofstream out(file_name, std::ios::binary);
    ASSERT_TRUE(out.is_open());
    const std::string data(16U, 'B');
    out.write(data.data(), static_cast<std::streamsize>(data.size()));
  }
  std::unique_ptr<char[]> buf(new char[64]);
  size_t left_size = 64U;
  // 仅声明 16 字节权重, 剩余 48 字节由拷贝函数清零, 避免暴露未初始化设备内存
  EXPECT_EQ(FileConstantUtils::CopyOneWeightFromFile(buf.get(), file_name, 0U, 16U, left_size), SUCCESS);
  EXPECT_EQ(left_size, 64U - 16U);
  (void)remove(file_name.c_str());
}

TEST_F(UtestFileConstantUtilTransfer, CopyOneWeightFromFileWithoutDeclaredLength) {
  // length 未声明的场景: 请求长度由张量尺寸推导(可能含对齐 padding), 允许文件偏短,
  // 读入可用部分后对剩余区域清零, 不能因为文件短就加载失败
  const std::string file_name = "test_no_declared_length_weight.bin";
  {
    std::ofstream out(file_name, std::ios::binary);
    ASSERT_TRUE(out.is_open());
    const std::string data(64U, 'D');
    out.write(data.data(), static_cast<std::streamsize>(data.size()));
  }
  std::unique_ptr<char[]> buf(new char[128]);
  size_t left_size = 128U;
  EXPECT_EQ(FileConstantUtils::CopyOneWeightFromFile(buf.get(), file_name, 0U, 128U, left_size,
                                                     /* enforce_range = */ false),
            SUCCESS);
  EXPECT_EQ(left_size, 128U - 64U);
  (void)remove(file_name.c_str());
}

TEST_F(UtestFileConstantUtilTransfer, CopyOneWeightFromFileClearTailRange) {
  const std::string file_name = "test_clear_tail_weight.bin";
  {
    std::ofstream out(file_name, std::ios::binary);
    ASSERT_TRUE(out.is_open());
    const std::string data(16U, 'C');
    out.write(data.data(), static_cast<std::streamsize>(data.size()));
  }
  std::unique_ptr<char[]> buf(new char[1024]);
  // acl 桩对 maxCount == 321 的 memset 返回失败: 337 - 16 == 321, 用该组合精确验证
  // 尾部清零范围是 [本次读入量, 目标缓冲总大小)
  size_t left_size = 337U;
  Status ret = FileConstantUtils::CopyOneWeightFromFile(buf.get(), file_name, 0U, 16U, left_size);
  EXPECT_NE(ret, SUCCESS);           // 说明尾部清零确实被调用且 maxCount 按剩余空间计算
  EXPECT_EQ(left_size, 337U - 16U);  // 失败路径仍按实际读入量回填剩余空间

  // 对照: maxCount 变为 320 时不命中桩的失败注入, 说明清零范围随缓冲大小线性变化
  left_size = 336U;
  ret = FileConstantUtils::CopyOneWeightFromFile(buf.get(), file_name, 0U, 16U, left_size);
  EXPECT_EQ(ret, SUCCESS);
  EXPECT_EQ(left_size, 336U - 16U);
  (void)remove(file_name.c_str());
}

TEST_F(UtestFileConstantUtilTransfer, CopyOneWeightFromFileClearHostTail) {
  const std::string file_name = "test_clear_host_tail_weight.bin";
  {
    std::ofstream out(file_name, std::ios::binary);
    ASSERT_TRUE(out.is_open());
    const std::string data(16U, 'H');
    out.write(data.data(), static_cast<std::streamsize>(data.size()));
  }
  // buf 模拟 HostMemAllocator 分配的普通堆内存(AlignedPtr 底层为 new[]), 预填 0xFF 以验证尾部被真正清零
  std::unique_ptr<char[]> buf(new char[1024]);
  for (size_t i = 0U; i < 1024U; ++i) {
    buf[i] = static_cast<char>(0xFF);
  }
  // acl 桩对 maxCount == 321 的 aclrtMemset 返回失败(337 - 16 == 321), 用作"是否走了 ACL 接口"的探针:
  // Host 路径走 memset_s 不经过该桩故应成功; 若误用 aclrtMemset 清零 Host 内存则会命中注入而失败
  size_t left_size = 337U;
  const Status ret = FileConstantUtils::CopyOneWeightFromFile(buf.get(), file_name, 0U, 16U, left_size, true,
                                                              WeightMemLocation::kHost);
  EXPECT_EQ(ret, SUCCESS);
  EXPECT_EQ(left_size, 337U - 16U);
  EXPECT_EQ(buf[0U], 'H');
  EXPECT_EQ(buf[15U], 'H');
  bool tail_cleared = true;
  for (size_t i = 16U; i < 337U; ++i) {
    if (buf[i] != '\0') {
      tail_cleared = false;
      break;
    }
  }
  EXPECT_TRUE(tail_cleared);
  // 对照: 同尺寸下默认 Device 路径命中桩的失败注入, 证明两条分支确实调用了不同接口
  left_size = 337U;
  EXPECT_NE(FileConstantUtils::CopyOneWeightFromFile(buf.get(), file_name, 0U, 16U, left_size), SUCCESS);
  (void)remove(file_name.c_str());
}

TEST_F(UtestFileConstantUtilTransfer, GetFilePathOK) {
  std::map<std::string, std::string> options;
  options["ge.exec.value_bins"] =
      "{\"value_bins\":[{\"value_bin_id\":\"vector_search_buchet_value_bin\", \"value_bin_file\":\"hello.bin\"}]}";
  ge::GetThreadLocalContext().SetGraphOption(options);
  std::map<std::string, std::string> file_id_and_path_map;
  FileConstantUtils::GetFileIdToPathMapFromOption(file_id_and_path_map);
  OpDescPtr op_desc = CreateOpDesc("FileConstant", FILECONSTANT);
  std::vector<int64_t> shape = {2, 2, 2, 2};
  EXPECT_TRUE(AttrUtils::SetStr(op_desc, ATTR_NAME_FILE_CONSTANT_ID, "vector_search_buchet_value_bin"));
  std::string file_path;
  size_t offset = 0U;
  size_t length = 0U;
  Status ret = FileConstantUtils::GetFilePath(op_desc, file_id_and_path_map, file_path, offset, length);
  EXPECT_EQ(ret, SUCCESS);
  EXPECT_EQ(file_path, "hello.bin");
}

TEST_F(UtestFileConstantUtilTransfer, GetRealFilePathOK) {
  std::map<std::string, std::string> file_id_and_path_map;
  OpDescPtr op_desc = CreateOpDesc("FileConstant", FILECONSTANT);
  EXPECT_TRUE(AttrUtils::SetStr(op_desc, ATTR_NAME_FILE_PATH, "hello.bin"));
  std::string file_path;
  size_t offset = 0U;
  size_t length = 0U;
  Status ret = FileConstantUtils::GetFilePath(op_desc, file_id_and_path_map, file_path, offset, length);
  EXPECT_EQ(ret, SUCCESS);
  EXPECT_EQ(file_path, "hello.bin");
}

TEST_F(UtestFileConstantUtilTransfer, GetFileConstantPath) {
  OpDescPtr op_desc = CreateOpDesc("FileConstant", FILECONSTANT);
  EXPECT_TRUE(AttrUtils::SetInt(op_desc, ATTR_NAME_OFFSET, 64));
  EXPECT_TRUE(AttrUtils::SetInt(op_desc, ATTR_NAME_LENGTH, 1024));
  std::string file_path;
  size_t offset = 0U;
  size_t length = 0U;
  std::map<std::string, std::string> file_id_and_path_map;
  Status ret = FileConstantUtils::GetFilePath(op_desc, file_id_and_path_map, file_path, offset, length);
  EXPECT_NE(ret, SUCCESS);
  EXPECT_TRUE(AttrUtils::SetStr(op_desc, ATTR_NAME_LOCATION, "/tmp/hello.bin"));
  ret = FileConstantUtils::GetFilePath(op_desc, file_id_and_path_map, file_path, offset, length);
  EXPECT_EQ(ret, SUCCESS);
  EXPECT_EQ(file_path, "/tmp/hello.bin");
  EXPECT_EQ(offset, 64);
  EXPECT_EQ(length, 1024);
}

TEST_F(UtestFileConstantUtilTransfer, test_set_external_path_with_dir_success) {
  OpDescPtr op_desc = CreateOpDesc("file_const_0", FILECONSTANT);
  EXPECT_TRUE(AttrUtils::SetStr(op_desc, ATTR_NAME_LOCATION, "tmp/hello.bin"));
  std::string cache_dir = "cache_dir/";
  auto ret = FileConstantUtils::SetExternalPath(op_desc, cache_dir.append("weight/"));
  EXPECT_EQ(ret, SUCCESS);
  std::map<std::string, std::string> file_id_and_path_map;
  std::string file_path;
  size_t offset = 0U;
  size_t length = 0U;
  ret = FileConstantUtils::GetFilePath(op_desc, file_id_and_path_map, file_path, offset, length);
  EXPECT_EQ(ret, SUCCESS);
  EXPECT_EQ(file_path, "cache_dir/weight/hello.bin");
}

TEST_F(UtestFileConstantUtilTransfer, test_set_external_path_success) {
  OpDescPtr op_desc = CreateOpDesc("file_const_0", FILECONSTANT);
  std::string om_dir = "om_path/";
  std::string om_path = om_dir + "hello.om";
  const std::string kWeightDir = om_dir + "weight/";
  std::string weight_dir;
  EXPECT_NE(FileConstantUtils::GetExternalWeightDirFromOmPath(om_path, weight_dir), ge::SUCCESS);

  std::string file_name = "om_path/weight/hello.bin";
  size_t file_const_size = 100;
  std::unique_ptr<float[]> float_buf(new float[file_const_size / sizeof(float)]);
  Status ret = FileSaver::SaveToFile(file_name, float_buf.get(), file_const_size);
  EXPECT_EQ(ret, SUCCESS);
  ret = FileSaver::SaveToFile(om_path, float_buf.get(), file_const_size);
  EXPECT_EQ(ret, SUCCESS);

  EXPECT_TRUE(AttrUtils::SetStr(op_desc, ATTR_NAME_LOCATION, "hello.bin"));
  std::string file_path;
  size_t offset = 0U;
  size_t length = 0U;
  std::map<std::string, std::string> file_id_and_path_map;
  ret = FileConstantUtils::GetFilePath(op_desc, file_id_and_path_map, file_path, offset, length);
  EXPECT_EQ(ret, SUCCESS);
  EXPECT_EQ(file_path, "hello.bin");

  weight_dir = "";
  ge::ModelData model_data;
  model_data.weight_path = om_dir + "weight/";
  ASSERT_EQ(FileConstantUtils::GetExternalWeightDir(model_data, weight_dir), ge::SUCCESS);
  EXPECT_NE(weight_dir.find(kWeightDir), weight_dir.npos);
  ret = FileConstantUtils::SetExternalPath(op_desc, weight_dir);
  EXPECT_EQ(ret, SUCCESS);
  ret = FileConstantUtils::GetFilePath(op_desc, file_id_and_path_map, file_path, offset, length);
  EXPECT_EQ(ret, SUCCESS);
  EXPECT_NE(file_path.find("om_path/weight/hello.bin"), file_path.npos);

  weight_dir = "";
  ASSERT_EQ(FileConstantUtils::GetExternalWeightDirFromOmPath(om_path, weight_dir), ge::SUCCESS);
  EXPECT_NE(weight_dir.find(kWeightDir), weight_dir.npos);
  ret = FileConstantUtils::SetExternalPath(op_desc, weight_dir);
  EXPECT_EQ(ret, SUCCESS);
  ret = FileConstantUtils::GetFilePath(op_desc, file_id_and_path_map, file_path, offset, length);
  EXPECT_EQ(ret, SUCCESS);
  EXPECT_NE(file_path.find("om_path/weight/hello.bin"), file_path.npos);
  ge::ut::GraphBuilder builder("graph");
  auto graph = builder.GetGraph();
  om_path = "om_path/hello.om";
  weight_dir = "";
  ASSERT_EQ(FileConstantUtils::GetExternalWeightDirFromOmPath(om_path, weight_dir), ge::SUCCESS);
  EXPECT_NE(weight_dir.find(kWeightDir), weight_dir.npos);
  ret = FileConstantUtils::SetExternalPath(graph, weight_dir);
  EXPECT_EQ(ret, SUCCESS);
  weight_dir = "";
  ASSERT_EQ(FileConstantUtils::GetExternalWeightDirFromOmPath("", weight_dir), ge::SUCCESS);
  EXPECT_EQ(weight_dir, "");
  (void)mmRmdir("om_path");
}

TEST_F(UtestFileConstantUtilTransfer, test_convert_file_const_to_const_success) {
  std::string file_name = "tmp_weight_pid/graph1/hello.bin";
  size_t file_const_size = 12;
  size_t value_num = file_const_size / sizeof(float);
  std::unique_ptr<float[]> float_buf(new float[value_num]);
  for (size_t i = 0U; i < value_num; i++) {
    float_buf[i] = static_cast<float>(i);
  }
  Status ret = FileSaver::SaveToFile(file_name, float_buf.get(), file_const_size);
  EXPECT_EQ(ret, SUCCESS);

  auto builder = ut::GraphBuilder("graph1");
  auto file_const = builder.AddNode("file_const", FILECONSTANT, 0, 1, FORMAT_ND, DT_FLOAT, {3});
  auto netoutput = builder.AddNode("Node_OutPut", "NetOutPut", 1, 0);
  OpDescPtr op_desc = file_const->GetOpDesc();
  EXPECT_TRUE(AttrUtils::SetStr(op_desc, ATTR_NAME_LOCATION, file_name));
  (void)AttrUtils::SetDataType(op_desc, "dtype", DT_FLOAT);

  builder.AddDataEdge(file_const, 0, netoutput, 0);
  auto graph = builder.GetGraph();
  ret = FileConstantUtils::ConvertFileConstToConst(graph);
  EXPECT_EQ(ret, SUCCESS);
  auto const_node = graph->FindNode("file_const");
  EXPECT_EQ(const_node->GetType(), CONSTANT);
  const auto &weight = OpDescUtils::MutableWeights(const_node);
  auto const_value_size = weight[0]->GetData().GetSize();
  EXPECT_EQ(const_value_size, 12);
  auto const_value_data = reinterpret_cast<const float *>(weight[0]->GetData().GetData())[0];
  EXPECT_EQ(const_value_data, 0.0f);
  const_value_data = reinterpret_cast<const float *>(weight[0]->GetData().GetData())[1];
  EXPECT_EQ(const_value_data, 1.0f);
  const_value_data = reinterpret_cast<const float *>(weight[0]->GetData().GetData())[2];
  EXPECT_EQ(const_value_data, 2.0f);
  (void)mmRmdir("tmp_weight_pid");
}

TEST_F(UtestFileConstantUtilTransfer, ConvertConstToFileConst_Ok_MultipleModelSameConst) {
  const auto build_graph = []() {
    ge::ut::GraphBuilder builder("graph");
    auto const1 = builder.AddNode("const1", "Const", 0, 1);
    auto netoutput = builder.AddNode("Node_OutPut", "NetOutPut", 1, 0);
    ge::GeTensorPtr tensor = std::make_shared<GeTensor>();
    std::vector<uint8_t> value(4 * 8 * 8);
    std::vector<int64_t> shape{1, 4, 8, 8};
    tensor->MutableTensorDesc().SetShape(GeShape(shape));
    tensor->SetData(value);
    tensor->MutableTensorDesc().SetDataType(DT_UINT8);
    ConstantUtils::SetWeight(const1->GetOpDesc(), 0, tensor);
    (void)AttrUtils::SetStr(const1->GetOpDesc(), ATTR_NAME_WEIGHT_SHA256, "1234567");
    builder.AddDataEdge(const1, 0, netoutput, 0);
    return builder.GetGraph();
  };
  const auto &external_weight_manager = ExternalWeightManagerPool::Instance().GetManager(GetContext().SessionId());
  ASSERT_NE(external_weight_manager, nullptr);
  external_weight_manager->SetWeightPath("./om_temp/weight");
  auto &meta = external_weight_manager->MutableMetaFile();
  meta.hash_to_weight_file.clear();
  // 第一次保存，没有权重
  auto graph_1 = build_graph();
  auto ret = FileConstantUtils::ConvertConstToFileConst(graph_1);
  EXPECT_EQ(ret, SUCCESS);
  EXPECT_EQ(meta.hash_to_weight_file.size(), 1);
  (void)mmRmdir("./om_temp");

  // 第二次保存相同模型，meta中有权重信息，但是没有对应权重文件，重新落盘
  auto graph_2 = build_graph();
  ret = FileConstantUtils::ConvertConstToFileConst(graph_2);
  EXPECT_EQ(ret, SUCCESS);
  EXPECT_EQ(mmAccess("./om_temp/weight/weight_1234567"), EOK);
  meta.hash_to_weight_file.clear();
  (void)mmRmdir("./om_temp");
}

TEST_F(UtestFileConstantUtilTransfer, GetAllConstNodesAndWeightHash_ReturnsOrderedResult) {
  ge::ut::GraphBuilder builder("graph");
  auto const3 = builder.AddNode("const3", "Const", 0, 1);
  auto const1 = builder.AddNode("const1", "Const", 0, 1);
  auto const2 = builder.AddNode("const2", "Const", 0, 1);
  auto const1_duplicate = builder.AddNode("const1", "Const", 0, 1);

  ge::GeTensorPtr tensor = std::make_shared<GeTensor>();
  std::vector<uint8_t> value(4 * 8 * 8);
  std::vector<int64_t> shape{1, 4, 8, 8};
  tensor->MutableTensorDesc().SetShape(GeShape(shape));
  tensor->SetData(value);
  tensor->MutableTensorDesc().SetDataType(DT_UINT8);

  ConstantUtils::SetWeight(const3->GetOpDesc(), 0, tensor);
  ConstantUtils::SetWeight(const1->GetOpDesc(), 0, tensor);
  ConstantUtils::SetWeight(const2->GetOpDesc(), 0, tensor);
  ConstantUtils::SetWeight(const1_duplicate->GetOpDesc(), 0, tensor);
  (void)AttrUtils::SetStr(const3->GetOpDesc(), ATTR_NAME_WEIGHT_SHA256, "hash3");
  (void)AttrUtils::SetStr(const1->GetOpDesc(), ATTR_NAME_WEIGHT_SHA256, "hash1");
  (void)AttrUtils::SetStr(const2->GetOpDesc(), ATTR_NAME_WEIGHT_SHA256, "hash2");
  (void)AttrUtils::SetStr(const1_duplicate->GetOpDesc(), ATTR_NAME_WEIGHT_SHA256, "hash1_dup");

  const auto graph = builder.GetGraph();
  const1->GetOpDesc()->SetId(1);
  const1_duplicate->GetOpDesc()->SetId(2);
  const2->GetOpDesc()->SetId(3);
  const3->GetOpDesc()->SetId(4);
  const auto const_to_weight_hash_list = FileConstantUtils::GetAllConstNodesAndWeightHash(graph);

  std::vector<std::string> ordered_names;
  std::vector<std::string> ordered_hashes;
  for (const auto &const_and_weight_hash : const_to_weight_hash_list) {
    ordered_names.emplace_back(const_and_weight_hash.first->GetName());
    ordered_hashes.emplace_back(const_and_weight_hash.second.second);
  }

  const std::vector<std::string> expected_names = {"const1", "const1", "const2", "const3"};
  const std::vector<std::string> expected_hashes = {"hash1", "hash1_dup", "hash2", "hash3"};
  EXPECT_EQ(ordered_names, expected_names);
  EXPECT_EQ(ordered_hashes, expected_hashes);
}

TEST_F(UtestFileConstantUtilTransfer, GetAllConstNodesAndWeightHash_OrdersSameNameNodesByOwnerGraph) {
  ge::ut::GraphBuilder root_builder("z_graph");
  const auto root_const = root_builder.AddNode("same_const", "Const", 0, 1);
  (void)root_builder.AddNode("partitioned_call", "PartitionedCall", 0, 0);
  const auto root_graph = root_builder.GetGraph();

  ge::ut::GraphBuilder subgraph_builder("a_graph");
  const auto subgraph_const = subgraph_builder.AddNode("same_const", "Const", 0, 1);
  const auto subgraph = subgraph_builder.GetGraph();
  ge::ut::GraphBuilder::AddPartitionedCall(root_graph, "partitioned_call", subgraph);

  ge::GeTensorPtr tensor = std::make_shared<GeTensor>();
  std::vector<uint8_t> value(4 * 8 * 8);
  std::vector<int64_t> shape{1, 4, 8, 8};
  tensor->MutableTensorDesc().SetShape(GeShape(shape));
  tensor->SetData(value);
  tensor->MutableTensorDesc().SetDataType(DT_UINT8);

  ConstantUtils::SetWeight(root_const->GetOpDesc(), 0, tensor);
  ConstantUtils::SetWeight(subgraph_const->GetOpDesc(), 0, tensor);
  (void)AttrUtils::SetStr(root_const->GetOpDesc(), ATTR_NAME_WEIGHT_SHA256, "root_hash");
  (void)AttrUtils::SetStr(subgraph_const->GetOpDesc(), ATTR_NAME_WEIGHT_SHA256, "subgraph_hash");

  const auto const_to_weight_hash_list = FileConstantUtils::GetAllConstNodesAndWeightHash(root_graph);

  ASSERT_EQ(const_to_weight_hash_list.size(), 2U);
  EXPECT_EQ(const_to_weight_hash_list[0].second.second, "subgraph_hash");
  EXPECT_EQ(const_to_weight_hash_list[1].second.second, "root_hash");
}

TEST_F(UtestFileConstantUtilTransfer, GetAllConstNodesAndWeightHash_SkipsEmptyAndMissingHashNodes) {
  const auto empty_graph = MakeShared<ComputeGraph>("empty_graph");
  ASSERT_NE(empty_graph, nullptr);
  EXPECT_TRUE(FileConstantUtils::GetAllConstNodesAndWeightHash(empty_graph).empty());

  ge::ut::GraphBuilder builder("graph");
  (void)builder.AddNode("const_without_hash", "Const", 0, 1);
  (void)builder.AddNode("relu", "Relu", 1, 1);
  const auto graph = builder.GetGraph();

  EXPECT_TRUE(FileConstantUtils::GetAllConstNodesAndWeightHash(graph).empty());
}

TEST_F(UtestFileConstantUtilTransfer, test_convert_const_to_file_const_AscendWorkPath_success) {
  ge::ut::GraphBuilder builder("graph");
  auto const1 = builder.AddNode("const1", "Const", 0, 1);
  auto netoutput = builder.AddNode("Node_OutPut", "NetOutPut", 1, 0);
  ge::GeTensorPtr tensor = std::make_shared<GeTensor>();
  std::vector<uint8_t> value(4 * 8 * 8);
  std::vector<int64_t> shape{1, 4, 8, 8};
  tensor->MutableTensorDesc().SetShape(GeShape(shape));
  tensor->SetData(value);
  tensor->MutableTensorDesc().SetDataType(DT_UINT8);
  ConstantUtils::SetWeight(const1->GetOpDesc(), 0, tensor);
  (void)AttrUtils::SetStr(const1->GetOpDesc(), ATTR_NAME_WEIGHT_SHA256, "1234567");

  builder.AddDataEdge(const1, 0, netoutput, 0);
  auto graph = builder.GetGraph();

  ge::char_t current_path[MMPA_MAX_PATH] = {'\0'};
  getcwd(current_path, MMPA_MAX_PATH);
  mmSetEnv("ASCEND_WORK_PATH", current_path, 1);
  auto ret = FileConstantUtils::ConvertConstToFileConst(graph);
  EXPECT_EQ(ret, SUCCESS);

  std::string file_const_path = current_path;
  file_const_path += "/tmp_weight_" + std::to_string(getpid()) + "_0";
  EXPECT_EQ(mmAccess(file_const_path.c_str()), EN_OK);
  unsetenv("ASCEND_WORK_PATH");
}

TEST_F(UtestFileConstantUtilTransfer, test_convert_const_to_file_const_no_const) {
  ge::ut::GraphBuilder builder("graph");
  auto netoutput = builder.AddNode("Node_OutPut", "NetOutPut", 1, 0);

  auto graph = builder.GetGraph();
  auto ret = FileConstantUtils::ConvertConstToFileConst(graph);
  EXPECT_EQ(ret, SUCCESS);
}

TEST_F(UtestFileConstantUtilTransfer, test_read_json_failed) {
  MmpaStub::GetInstance().SetImpl(std::make_shared<MockMmpa>());
  ge::ut::GraphBuilder builder("graph");
  auto const1 = builder.AddNode("const1", "Const", 0, 1);
  auto netoutput = builder.AddNode("Node_OutPut", "NetOutPut", 1, 0);
  ge::GeTensorPtr tensor = std::make_shared<GeTensor>();
  std::vector<uint8_t> value(4 * 8 * 8);
  std::vector<int64_t> shape{1, 4, 8, 8};
  tensor->MutableTensorDesc().SetShape(GeShape(shape));
  tensor->SetData(value);
  tensor->MutableTensorDesc().SetDataType(DT_UINT8);
  ConstantUtils::SetWeight(const1->GetOpDesc(), 0, tensor);
  (void)AttrUtils::SetStr(const1->GetOpDesc(), ATTR_NAME_WEIGHT_SHA256, "1234567");

  builder.AddDataEdge(const1, 0, netoutput, 0);
  auto graph = builder.GetGraph();
  auto ret = FileConstantUtils::ConvertConstToFileConst(graph);
  EXPECT_EQ(ret, FAILED);
  MmpaStub::GetInstance().Reset();
}

TEST_F(UtestFileConstantUtilTransfer, test_flock_failed) {
  MmpaStub::GetInstance().SetImpl(std::make_shared<MockMmpaForFlockFailed>());
  ge::ut::GraphBuilder builder("graph");
  auto const1 = builder.AddNode("const1", "Const", 0, 1);
  auto netoutput = builder.AddNode("Node_OutPut", "NetOutPut", 1, 0);
  ge::GeTensorPtr tensor = std::make_shared<GeTensor>();
  std::vector<uint8_t> value(4 * 8 * 8);
  std::vector<int64_t> shape{1, 4, 8, 8};
  tensor->MutableTensorDesc().SetShape(GeShape(shape));
  tensor->SetData(value);
  tensor->MutableTensorDesc().SetDataType(DT_UINT8);
  ConstantUtils::SetWeight(const1->GetOpDesc(), 0, tensor);
  (void)AttrUtils::SetStr(const1->GetOpDesc(), ATTR_NAME_WEIGHT_SHA256, "1234567");

  builder.AddDataEdge(const1, 0, netoutput, 0);
  auto graph = builder.GetGraph();
  auto ret = FileConstantUtils::ConvertConstToFileConst(graph);
  EXPECT_EQ(ret, FAILED);
  MmpaStub::GetInstance().Reset();
}

TEST_F(UtestFileConstantUtilTransfer, test_convert_const_to_file_const_specify_file_path_reuse) {
  ge::ut::GraphBuilder builder("graph");
  auto const1 = builder.AddNode("const1", "Const", 0, 1);
  auto const2 = builder.AddNode("const2", "Const", 0, 1);
  auto netoutput = builder.AddNode("Node_OutPut", "NetOutPut", 1, 0);
  ge::GeTensorPtr tensor = std::make_shared<GeTensor>();
  std::vector<uint8_t> value(4 * 8 * 8);
  std::vector<int64_t> shape{1, 4, 8, 8};
  tensor->MutableTensorDesc().SetShape(GeShape(shape));
  tensor->SetData(value);
  tensor->MutableTensorDesc().SetDataType(DT_UINT8);
  ConstantUtils::SetWeight(const1->GetOpDesc(), 0, tensor);
  ConstantUtils::SetWeight(const2->GetOpDesc(), 0, tensor);
  (void)AttrUtils::SetStr(const1->GetOpDesc(), ATTR_NAME_WEIGHT_SHA256, "1234567");
  (void)AttrUtils::SetStr(const2->GetOpDesc(), ATTR_NAME_WEIGHT_SHA256, "1234567");

  builder.AddDataEdge(const1, 0, netoutput, 0);
  auto graph = builder.GetGraph();
  const auto &external_weight_manager = ExternalWeightManagerPool::Instance().GetManager(GetContext().SessionId());
  EXPECT_EQ(external_weight_manager != nullptr, true);
  external_weight_manager->SetWeightPath("./om_out/weight");
  auto ret = FileConstantUtils::ConvertConstToFileConst(graph);
  EXPECT_EQ(ret, SUCCESS);
  auto fileconstant1 = graph->FindFirstNodeMatchType(FILECONSTANT);
  ASSERT_NE(fileconstant1, nullptr);

  // reuse with meta.json
  ge::ut::GraphBuilder builder1("graph");
  auto const1_reuse = builder1.AddNode("const3", "Const", 0, 1);
  auto netoutput1 = builder1.AddNode("Node_OutPut", "NetOutPut", 1, 0);
  ConstantUtils::SetWeight(const1_reuse->GetOpDesc(), 0, tensor);
  (void)AttrUtils::SetStr(const1_reuse->GetOpDesc(), ATTR_NAME_WEIGHT_SHA256, "1234567");
  builder1.AddDataEdge(const1_reuse, 0, netoutput1, 0);
  auto graph1 = builder1.GetGraph();
  ret = FileConstantUtils::ConvertConstToFileConst(graph1);
  EXPECT_EQ(ret, SUCCESS);
  (void)mmRmdir("om_out");
}

TEST_F(UtestFileConstantUtilTransfer, test_convert_const_to_file_const_specify_file_path_reuse_external_weight_dir) {
  ge::ut::GraphBuilder builder("graph");
  auto const1 = builder.AddNode("const1", "Const", 0, 1);
  auto const2 = builder.AddNode("const2", "Const", 0, 1);
  auto netoutput = builder.AddNode("Node_OutPut", "NetOutPut", 1, 0);
  ge::GeTensorPtr tensor = std::make_shared<GeTensor>();
  std::vector<uint8_t> value(4 * 8 * 8);
  std::vector<int64_t> shape{1, 4, 8, 8};
  tensor->MutableTensorDesc().SetShape(GeShape(shape));
  tensor->SetData(value);
  tensor->MutableTensorDesc().SetDataType(DT_UINT8);
  ConstantUtils::SetWeight(const1->GetOpDesc(), 0, tensor);
  ConstantUtils::SetWeight(const2->GetOpDesc(), 0, tensor);
  (void)AttrUtils::SetStr(const1->GetOpDesc(), ATTR_NAME_WEIGHT_SHA256, "1234567");
  (void)AttrUtils::SetStr(const2->GetOpDesc(), ATTR_NAME_WEIGHT_SHA256, "1234567");

  builder.AddDataEdge(const1, 0, netoutput, 0);
  auto graph = builder.GetGraph();
  const auto &external_weight_manager = ExternalWeightManagerPool::Instance().GetManager(GetContext().SessionId());
  EXPECT_EQ(external_weight_manager != nullptr, true);
  std::string option_str = ExternalWeightManager::GetWeightPathFromOption();
  std::cout << "test_convert_const_to_file_const_specify_file_path_reuse_external_weight_dir GetWeightPathFromOption "
               "option_str: "
            << option_str << "end" << std::endl;
  std::cout << "test_convert_const_to_file_const_specify_file_path_reuse_external_weight_dir weightPath begin."
            << std::endl;
  std::string weightPath = ExternalWeightManager::GetWeightPathFromOption();
  std::cout << "test_convert_const_to_file_const_specify_file_path_reuse_external_weight_dir GetWeightPathFromOption "
               "weightPath: "
            << weightPath << "end" << std::endl;
  external_weight_manager->SetWeightPath("ge.externalWeightDir");
  auto ret = FileConstantUtils::ConvertConstToFileConst(graph);
  EXPECT_EQ(ret, SUCCESS);
  auto fileconstant1 = graph->FindFirstNodeMatchType(FILECONSTANT);
  ASSERT_NE(fileconstant1, nullptr);

  // reuse with meta.json
  ge::ut::GraphBuilder builder1("graph");
  auto const1_reuse = builder1.AddNode("const3", "Const", 0, 1);
  auto netoutput1 = builder1.AddNode("Node_OutPut", "NetOutPut", 1, 0);
  ConstantUtils::SetWeight(const1_reuse->GetOpDesc(), 0, tensor);
  (void)AttrUtils::SetStr(const1_reuse->GetOpDesc(), ATTR_NAME_WEIGHT_SHA256, "1234567");
  builder1.AddDataEdge(const1_reuse, 0, netoutput1, 0);
  auto graph1 = builder1.GetGraph();
  ret = FileConstantUtils::ConvertConstToFileConst(graph1);
  EXPECT_EQ(ret, SUCCESS);
  (void)mmRmdir("om_out");
}

TEST_F(UtestFileConstantUtilTransfer, test_convert_const_to_file_const_small_weight_size) {
  ge::ut::GraphBuilder builder("graph");
  auto const1 = builder.AddNode("const1", "Const", 0, 1);
  auto netoutput = builder.AddNode("Node_OutPut", "NetOutPut", 1, 0);
  ge::GeTensorPtr tensor = std::make_shared<GeTensor>();
  std::vector<uint8_t> value{1, 2, 3};
  std::vector<int64_t> shape{3};
  tensor->MutableTensorDesc().SetShape(GeShape(shape));
  tensor->SetData(value);
  tensor->MutableTensorDesc().SetDataType(DT_UINT8);
  ConstantUtils::SetWeight(const1->GetOpDesc(), 0, tensor);

  builder.AddDataEdge(const1, 0, netoutput, 0);
  auto graph = builder.GetGraph();
  auto ret = FileConstantUtils::ConvertConstToFileConst(graph);
  EXPECT_EQ(ret, SUCCESS);
}

TEST_F(UtestFileConstantUtilTransfer, test_convert_const_to_file_const_empty_tensor) {
  ge::ut::GraphBuilder builder("graph");
  auto const1 = builder.AddNode("const1", "Const", 0, 1);
  auto netoutput = builder.AddNode("Node_OutPut", "NetOutPut", 1, 0);

  builder.AddDataEdge(const1, 0, netoutput, 0);
  auto graph = builder.GetGraph();
  auto ret = FileConstantUtils::ConvertConstToFileConst(graph);
  EXPECT_EQ(ret, SUCCESS);
  ret = FileConstantUtils::ConvertConstToFileConst(const1);
  EXPECT_EQ(ret, SUCCESS);
}

TEST_F(UtestFileConstantUtilTransfer, test_change_file_path_success) {
  std::string file_name = "tmp_weight_pid/weight.bin";
  size_t file_const_size = 12;
  size_t value_num = file_const_size / sizeof(float);
  std::unique_ptr<float[]> float_buf(new float[value_num]);
  for (size_t i = 0U; i < value_num; i++) {
    float_buf[i] = static_cast<float>(i);
  }
  Status ret = FileSaver::SaveToFile(file_name, float_buf.get(), file_const_size);
  EXPECT_EQ(ret, SUCCESS);

  auto builder = ut::GraphBuilder("graph1");
  auto file_const = builder.AddNode("file_const", FILECONSTANT, 0, 1, FORMAT_ND, DT_FLOAT, {3});
  auto netoutput = builder.AddNode("Node_OutPut", "NetOutPut", 1, 0);
  OpDescPtr op_desc = file_const->GetOpDesc();
  EXPECT_TRUE(AttrUtils::SetStr(op_desc, ATTR_NAME_LOCATION, file_name));
  (void)AttrUtils::SetDataType(op_desc, "dtype", DT_FLOAT);

  builder.AddDataEdge(file_const, 0, netoutput, 0);
  auto graph = builder.GetGraph();
  std::string om_path = "om_path/hello.om";
  ret = FileConstantUtils::ChangeFilePath(graph, om_path);
  EXPECT_EQ(ret, SUCCESS);
  std::string weight_file;
  EXPECT_TRUE(AttrUtils::GetStr(op_desc, ATTR_NAME_LOCATION, weight_file));
  EXPECT_EQ(weight_file, "weight.bin");
  (void)mmRmdir("om_path");
}

namespace {
struct DualFilePathGraphsForUt {
  ComputeGraphPtr main_graph;
  ComputeGraphPtr fallback_graph;
  OpDescPtr main_fc_desc;
  OpDescPtr fallback_shared_desc;
  OpDescPtr fallback_only_desc;
};

DualFilePathGraphsForUt BuildDualFilePathGraphsForUt(const std::string &shared_path,
                                                     const std::string &fallback_only_path) {
  DualFilePathGraphsForUt graphs;
  ut::GraphBuilder main_builder("main_graph");
  auto main_fc = main_builder.AddNode("main_fc", FILECONSTANT, 0, 1, FORMAT_ND, DT_FLOAT, {3});
  auto main_out = main_builder.AddNode("main_out", "NetOutPut", 1, 0);
  main_builder.AddDataEdge(main_fc, 0, main_out, 0);
  graphs.main_fc_desc = main_fc->GetOpDesc();
  (void)AttrUtils::SetStr(graphs.main_fc_desc, ATTR_NAME_LOCATION, shared_path);
  (void)AttrUtils::SetDataType(graphs.main_fc_desc, "dtype", DT_FLOAT);
  graphs.main_graph = main_builder.GetGraph();

  ut::GraphBuilder fallback_builder("fallback_graph");
  auto fallback_shared_fc =
      fallback_builder.AddNode("fallback_shared_fc", FILECONSTANT, 0, 1, FORMAT_ND, DT_FLOAT, {3});
  auto fallback_only_fc = fallback_builder.AddNode("fallback_only_fc", FILECONSTANT, 0, 1, FORMAT_ND, DT_FLOAT, {3});
  auto fallback_out = fallback_builder.AddNode("fallback_out", "NetOutPut", 2, 0);
  fallback_builder.AddDataEdge(fallback_shared_fc, 0, fallback_out, 0);
  fallback_builder.AddDataEdge(fallback_only_fc, 0, fallback_out, 1);
  graphs.fallback_shared_desc = fallback_shared_fc->GetOpDesc();
  graphs.fallback_only_desc = fallback_only_fc->GetOpDesc();
  (void)AttrUtils::SetStr(graphs.fallback_shared_desc, ATTR_NAME_LOCATION, shared_path);
  (void)AttrUtils::SetStr(graphs.fallback_only_desc, ATTR_NAME_LOCATION, fallback_only_path);
  (void)AttrUtils::SetDataType(graphs.fallback_shared_desc, "dtype", DT_FLOAT);
  (void)AttrUtils::SetDataType(graphs.fallback_only_desc, "dtype", DT_FLOAT);
  graphs.fallback_graph = fallback_builder.GetGraph();
  return graphs;
}
}  // namespace

TEST_F(UtestFileConstantUtilTransfer, test_change_file_path_for_dual_graphs) {
  const std::string main_dir = "tmp_weight_dual_main";
  const std::string fallback_dir = "tmp_weight_dual_fallback";
  ASSERT_EQ(CreateDirectory(main_dir), 0);
  ASSERT_EQ(CreateDirectory(fallback_dir), 0);
  const std::string shared_path = main_dir + "/shared.bin";
  const std::string fallback_only_path = fallback_dir + "/fallback_only.bin";
  {
    std::ofstream shared_ofs(shared_path, std::ios::binary);
    shared_ofs << "shared";
    std::ofstream fallback_ofs(fallback_only_path, std::ios::binary);
    fallback_ofs << "fallback";
  }

  auto graphs = BuildDualFilePathGraphsForUt(shared_path, fallback_only_path);
  const std::string om_path = "om_dual_path/hello.om";
  ASSERT_EQ(FileConstantUtils::ChangeFilePath(graphs.main_graph, graphs.fallback_graph, om_path), SUCCESS);

  std::string main_location;
  std::string fallback_shared_location;
  std::string fallback_only_location;
  EXPECT_TRUE(AttrUtils::GetStr(graphs.main_fc_desc, ATTR_NAME_LOCATION, main_location));
  EXPECT_TRUE(AttrUtils::GetStr(graphs.fallback_shared_desc, ATTR_NAME_LOCATION, fallback_shared_location));
  EXPECT_TRUE(AttrUtils::GetStr(graphs.fallback_only_desc, ATTR_NAME_LOCATION, fallback_only_location));
  EXPECT_EQ(main_location, "shared.bin");
  EXPECT_EQ(fallback_shared_location, "shared.bin");
  EXPECT_EQ(fallback_only_location, "fallback_only.bin");

  const std::string weight_dir = om_path.substr(0U, om_path.rfind('/')) + "/weight";
  EXPECT_EQ(mmAccess((weight_dir + "/shared.bin").c_str()), EN_OK);
  EXPECT_EQ(mmAccess((weight_dir + "/fallback_only.bin").c_str()), EN_OK);
  (void)remove((weight_dir + "/shared.bin").c_str());
  (void)remove((weight_dir + "/fallback_only.bin").c_str());
  (void)mmRmdir(weight_dir.c_str());
  (void)mmRmdir(om_path.substr(0U, om_path.rfind('/')).c_str());
  (void)mmRmdir(main_dir.c_str());
  (void)mmRmdir(fallback_dir.c_str());
}

TEST_F(UtestFileConstantUtilTransfer, test_reuse_external_weight_success) {
  GetContext().SetSessionId(0U);
  ge::ut::GraphBuilder builder("graph");
  auto const1 = builder.AddNode("const_1", "Const", 0, 1);
  auto const2 = builder.AddNode("const_2", "Const", 0, 1);
  auto const3 = builder.AddNode("const_1", "Const", 0, 1);
  auto netoutput = builder.AddNode("Node_OutPut", "NetOutPut", 3, 0);

  ge::GeTensorPtr tensor = std::make_shared<GeTensor>();
  std::vector<uint8_t> value(4 * 8 * 8);
  std::vector<int64_t> shape{1, 4, 8, 8};
  tensor->MutableTensorDesc().SetShape(GeShape(shape));
  tensor->SetData(value);
  tensor->MutableTensorDesc().SetDataType(DT_UINT8);
  ConstantUtils::SetWeight(const1->GetOpDesc(), 0, tensor);
  ConstantUtils::SetWeight(const2->GetOpDesc(), 0, tensor);
  ConstantUtils::SetWeight(const3->GetOpDesc(), 0, tensor);
  (void)AttrUtils::SetStr(const1->GetOpDesc(), ATTR_NAME_WEIGHT_SHA256, "1234567");
  (void)AttrUtils::SetStr(const2->GetOpDesc(), ATTR_NAME_WEIGHT_SHA256, "1234567");
  (void)AttrUtils::SetStr(const3->GetOpDesc(), ATTR_NAME_WEIGHT_SHA256, "1234567");

  builder.AddDataEdge(const1, 0, netoutput, 0);
  builder.AddDataEdge(const2, 0, netoutput, 1);
  builder.AddDataEdge(const3, 0, netoutput, 2);
  auto graph = builder.GetGraph();
  auto options_back = GetThreadLocalContext().GetAllGraphOptions();
  auto options = options_back;
  options[OPTION_GRAPH_COMPILER_CACHE_DIR] = "./cache_dir";
  GetThreadLocalContext().SetGraphOption(options);
  auto ret = FileConstantUtils::ConvertConstToFileConst(graph);
  EXPECT_EQ(ret, SUCCESS);
  auto fileconstant1 = graph->FindFirstNodeMatchType(FILECONSTANT);
  ASSERT_NE(fileconstant1, nullptr);
  const auto &fileconstant_info = FileConstantUtils::GetFileConstantInfo(fileconstant1->GetOpDesc());
  ASSERT_NE(fileconstant_info.weight_path, "");
  ExternalWeightManagerPool::Instance().Destroy();
  GetThreadLocalContext().SetGraphOption(options_back);
}

TEST_F(UtestFileConstantUtilTransfer, test_reuse_external_weight_combined_use_om_name_success) {
  GetContext().SetSessionId(0U);
  ge::ut::GraphBuilder builder("graph");
  auto const1 = builder.AddNode("const_1", "Const", 0, 1);
  auto const2 = builder.AddNode("const_2", "Const", 0, 1);
  auto const3 = builder.AddNode("const_1", "Const", 0, 1);
  auto netoutput = builder.AddNode("Node_OutPut", "NetOutPut", 3, 0);

  ge::GeTensorPtr tensor = std::make_shared<GeTensor>();
  std::vector<uint8_t> value(4 * 8 * 8);
  std::vector<int64_t> shape{1, 4, 8, 8};
  tensor->MutableTensorDesc().SetShape(GeShape(shape));
  tensor->SetData(value);
  tensor->MutableTensorDesc().SetDataType(DT_UINT8);
  ConstantUtils::SetWeight(const1->GetOpDesc(), 0, tensor);
  ConstantUtils::SetWeight(const2->GetOpDesc(), 0, tensor);
  ConstantUtils::SetWeight(const3->GetOpDesc(), 0, tensor);
  (void)AttrUtils::SetStr(const1->GetOpDesc(), ATTR_NAME_WEIGHT_SHA256, "1234567");
  (void)AttrUtils::SetStr(const2->GetOpDesc(), ATTR_NAME_WEIGHT_SHA256, "1234567");
  (void)AttrUtils::SetStr(const3->GetOpDesc(), ATTR_NAME_WEIGHT_SHA256, "1234567");

  builder.AddDataEdge(const1, 0, netoutput, 0);
  builder.AddDataEdge(const2, 0, netoutput, 1);
  builder.AddDataEdge(const3, 0, netoutput, 2);
  auto graph = builder.GetGraph();
  auto options_back = GetThreadLocalContext().GetAllGraphOptions();
  auto options = options_back;
  options[OPTION_GRAPH_COMPILER_CACHE_DIR] = "./cache_dir";
  GetThreadLocalContext().SetGraphOption(options);
  (void)AttrUtils::SetStr(graph, ATTR_MODEL_FILE_NAME_PREFIX, "./test_om_file.om");
  auto ret = FileConstantUtils::ConvertConstToFileConst(graph, true);
  EXPECT_EQ(ret, SUCCESS);
  auto fileconstant1 = graph->FindFirstNodeMatchType(FILECONSTANT);
  ASSERT_NE(fileconstant1, nullptr);
  const auto &fileconstant_info = FileConstantUtils::GetFileConstantInfo(fileconstant1->GetOpDesc());
  std::string weight_name = StringUtils::GetFileName(fileconstant_info.weight_path);
  ASSERT_EQ(weight_name, "test_om_file_weight_combined");
  ExternalWeightManagerPool::Instance().Destroy();
  GetThreadLocalContext().SetGraphOption(options_back);
}

TEST_F(UtestFileConstantUtilTransfer, test_reuse_external_weight_combined_use_graph_name_success) {
  GetContext().SetSessionId(0U);
  ge::ut::GraphBuilder builder("test_graph");
  auto const1 = builder.AddNode("const_1", "Const", 0, 1);
  auto const2 = builder.AddNode("const_2", "Const", 0, 1);
  auto const3 = builder.AddNode("const_1", "Const", 0, 1);
  auto netoutput = builder.AddNode("Node_OutPut", "NetOutPut", 3, 0);

  ge::GeTensorPtr tensor = std::make_shared<GeTensor>();
  std::vector<uint8_t> value(4 * 8 * 8);
  std::vector<int64_t> shape{1, 4, 8, 8};
  tensor->MutableTensorDesc().SetShape(GeShape(shape));
  tensor->SetData(value);
  tensor->MutableTensorDesc().SetDataType(DT_UINT8);
  ConstantUtils::SetWeight(const1->GetOpDesc(), 0, tensor);
  ConstantUtils::SetWeight(const2->GetOpDesc(), 0, tensor);
  ConstantUtils::SetWeight(const3->GetOpDesc(), 0, tensor);
  (void)AttrUtils::SetStr(const1->GetOpDesc(), ATTR_NAME_WEIGHT_SHA256, "1234567");
  (void)AttrUtils::SetStr(const2->GetOpDesc(), ATTR_NAME_WEIGHT_SHA256, "1234567");
  (void)AttrUtils::SetStr(const3->GetOpDesc(), ATTR_NAME_WEIGHT_SHA256, "1234567");

  builder.AddDataEdge(const1, 0, netoutput, 0);
  builder.AddDataEdge(const2, 0, netoutput, 1);
  builder.AddDataEdge(const3, 0, netoutput, 2);
  auto graph = builder.GetGraph();
  auto options_back = GetThreadLocalContext().GetAllGraphOptions();
  auto options = options_back;
  options[OPTION_GRAPH_COMPILER_CACHE_DIR] = "./cache_dir";
  GetThreadLocalContext().SetGraphOption(options);
  auto ret = FileConstantUtils::ConvertConstToFileConst(graph, true);
  EXPECT_EQ(ret, SUCCESS);
  auto fileconstant1 = graph->FindFirstNodeMatchType(FILECONSTANT);
  ASSERT_NE(fileconstant1, nullptr);
  const auto &fileconstant_info = FileConstantUtils::GetFileConstantInfo(fileconstant1->GetOpDesc());
  std::string weight_name = StringUtils::GetFileName(fileconstant_info.weight_path);
  ASSERT_EQ(weight_name, "test_graph_weight_combined");
  ExternalWeightManagerPool::Instance().Destroy();
  GetThreadLocalContext().SetGraphOption(options_back);
}

TEST_F(UtestFileConstantUtilTransfer, test_convert_const_to_file_const_with_DT_STRING) {
  ge::ut::GraphBuilder builder("graph");
  auto const1 = builder.AddNode("const1", "Const", 0, 1);
  auto netoutput = builder.AddNode("Node_OutPut", "NetOutPut", 1, 0);
  ge::GeTensorPtr tensor = std::make_shared<GeTensor>();
  std::vector<uint8_t> value{1, 2, 3};
  std::vector<int64_t> shape{3};
  tensor->MutableTensorDesc().SetShape(GeShape(shape));
  tensor->SetData(value);
  tensor->MutableTensorDesc().SetDataType(DT_UINT8);
  ConstantUtils::SetWeight(const1->GetOpDesc(), 0, tensor);
  const auto &output_desc = const1->GetOpDesc()->MutableOutputDesc(0U);
  output_desc->SetDataType(DT_STRING);

  builder.AddDataEdge(const1, 0, netoutput, 0);
  auto graph = builder.GetGraph();
  auto ret = FileConstantUtils::ConvertConstToFileConst(graph);
  EXPECT_EQ(ret, SUCCESS);
}

TEST_F(UtestFileConstantUtilTransfer, ConvertConstToFileConst_ConstNodeNotChangedForEmptyTensor) {
  ge::ut::GraphBuilder builder("graph");
  auto const1 = builder.AddNode("const1", "Const", 0, 1);
  auto netoutput = builder.AddNode("Node_OutPut", "NetOutPut", 1, 0);
  ge::GeTensorPtr tensor = std::make_shared<GeTensor>();
  std::vector<uint8_t> value{};
  std::vector<int64_t> shape{0};
  tensor->MutableTensorDesc().SetShape(GeShape(shape));
  tensor->SetData(value);
  tensor->MutableTensorDesc().SetDataType(DT_INT64);
  ConstantUtils::SetWeight(const1->GetOpDesc(), 0, tensor);
  const auto &output_desc = const1->GetOpDesc()->MutableOutputDesc(0U);
  output_desc->SetDataType(DT_INT64);
  builder.AddDataEdge(const1, 0, netoutput, 0);
  auto graph = builder.GetGraph();
  EXPECT_EQ(graph->FindFirstNodeMatchType(CONSTANT), const1);
  auto ret = FileConstantUtils::ConvertConstToFileConst(graph);
  EXPECT_EQ(ret, ge::SUCCESS);
  EXPECT_EQ(graph->FindFirstNodeMatchType(CONSTANT), const1);  // 空tensor的Const节点不会被转为FileConstant节点
}

TEST_F(UtestFileConstantUtilTransfer, test_refresh_relative_file_path_ok) {
  std::string file_name = "tmp_weight/hello.bin";
  auto builder = ut::GraphBuilder("graph");
  auto fileconst = builder.AddNode("file_const", FILECONSTANT, 0, 1);
  auto netoutput = builder.AddNode("Node_OutPut", "NetOutPut", 1, 0);
  OpDescPtr op_desc = fileconst->GetOpDesc();
  EXPECT_TRUE(AttrUtils::SetStr(op_desc, ATTR_NAME_LOCATION, file_name));
  builder.AddDataEdge(fileconst, 0, netoutput, 0);
  auto graph = builder.GetGraph();
  EXPECT_EQ(FileConstantUtils::RefreshRelativePath(graph), SUCCESS);
  EXPECT_TRUE(AttrUtils::GetStr(op_desc, ATTR_NAME_LOCATION, file_name));
  EXPECT_EQ(file_name, "hello.bin");
}

TEST_F(UtestFileConstantUtilTransfer, ReadJsonFile_Exception) {
  std::string bad_json_file = "test_bad_json_" + std::to_string(getpid()) + ".json";
  std::ofstream out(bad_json_file);
  out << "{invalid json content!!!}";
  out.close();
  nlohmann::json json_obj;
  std::ifstream file_stream(bad_json_file);
  bool caught = false;
  try {
    file_stream >> json_obj;
  } catch (const nlohmann::json::exception &) {
    caught = true;
  }
  EXPECT_TRUE(caught);
  (void)remove(bad_json_file.c_str());
}

TEST_F(UtestFileConstantUtilTransfer, from_json_OptionInfo) {
  nlohmann::json j;
  j["value_bins"] = nlohmann::json::array();
  OptionInfo option_info;
  from_json(j, option_info);
}

TEST_F(UtestFileConstantUtilTransfer, GetFilePath_FileIdNotFound) {
  std::map<std::string, std::string> file_id_to_path_map;
  file_id_to_path_map["other_id"] = "other.bin";
  OpDescPtr op_desc = CreateOpDesc("FileConstant", FILECONSTANT);
  EXPECT_TRUE(AttrUtils::SetStr(op_desc, ATTR_NAME_FILE_CONSTANT_ID, "nonexistent_id"));
  std::string file_path;
  size_t offset = 0U;
  size_t length = 0U;
  Status ret = FileConstantUtils::GetFilePath(op_desc, file_id_to_path_map, file_path, offset, length);
  EXPECT_EQ(ret, SUCCESS);
  EXPECT_TRUE(file_path.empty());
}

TEST_F(UtestFileConstantUtilTransfer, ConvertFileConstToConst_FallbackFilePath) {
  std::string file_name = "tmp_weight_pid/fallback_test.bin";
  size_t file_const_size = 12;
  size_t value_num = file_const_size / sizeof(float);
  std::unique_ptr<float[]> float_buf(new float[value_num]);
  for (size_t i = 0U; i < value_num; i++) {
    float_buf[i] = static_cast<float>(i);
  }
  Status ret = FileSaver::SaveToFile(file_name, float_buf.get(), file_const_size);
  EXPECT_EQ(ret, SUCCESS);

  auto builder = ut::GraphBuilder("graph1");
  auto file_const = builder.AddNode("file_const_fb", FILECONSTANT, 0, 1, FORMAT_ND, DT_FLOAT, {3});
  auto netoutput = builder.AddNode("Node_OutPut", "NetOutPut", 1, 0);
  OpDescPtr op_desc = file_const->GetOpDesc();
  EXPECT_TRUE(AttrUtils::SetStr(op_desc, ATTR_NAME_FILE_PATH, file_name));
  (void)AttrUtils::SetDataType(op_desc, "dtype", DT_FLOAT);

  builder.AddDataEdge(file_const, 0, netoutput, 0);
  auto graph = builder.GetGraph();
  ret = FileConstantUtils::ConvertFileConstToConst(file_const);
  EXPECT_EQ(ret, SUCCESS);
  EXPECT_EQ(file_const->GetType(), CONSTANT);
  (void)mmRmdir("tmp_weight_pid");
}

TEST_F(UtestFileConstantUtilTransfer, ConvertConstToFileConst_SingleNode) {
  ExternalWeightManagerPool::Instance().Destroy();
  GetContext().SetSessionId(0U);
  ge::ut::GraphBuilder builder("graph");
  auto const1 = builder.AddNode("const_single", "Const", 0, 1);
  ge::GeTensorPtr tensor = std::make_shared<GeTensor>();
  std::vector<uint8_t> value(4 * sizeof(float));
  std::vector<int64_t> shape{4};
  tensor->MutableTensorDesc().SetShape(GeShape(shape));
  tensor->SetData(value);
  tensor->MutableTensorDesc().SetDataType(DT_FLOAT);
  ConstantUtils::SetWeight(const1->GetOpDesc(), 0, tensor);

  const auto &external_weight_manager = ExternalWeightManagerPool::Instance().GetManager(GetContext().SessionId());
  ASSERT_NE(external_weight_manager, nullptr);
  external_weight_manager->SetWeightPath("./test_single_node_weight/weight");

  auto ret = FileConstantUtils::ConvertConstToFileConst(const1);
  EXPECT_EQ(ret, SUCCESS);
  EXPECT_EQ(const1->GetType(), FILECONSTANT);
  (void)mmRmdir("./test_single_node_weight");
  ExternalWeightManagerPool::Instance().Destroy();
}

TEST_F(UtestFileConstantUtilTransfer, ChangeFilePathAttr_EmptyAndNoTmpDir) {
  auto builder = ut::GraphBuilder("graph1");
  auto file_const1 = builder.AddNode("fc_empty", FILECONSTANT, 0, 1, FORMAT_ND, DT_FLOAT, {3});
  auto netoutput = builder.AddNode("Node_OutPut", "NetOutPut", 1, 0);

  OpDescPtr op1 = file_const1->GetOpDesc();
  EXPECT_TRUE(AttrUtils::SetStr(op1, ATTR_NAME_LOCATION, ""));

  builder.AddDataEdge(file_const1, 0, netoutput, 0);
  auto graph = builder.GetGraph();
  std::string om_path = "om_path/hello.om";
  auto ret = FileConstantUtils::ChangeFilePath(graph, om_path);
  EXPECT_EQ(ret, SUCCESS);
  (void)mmRmdir("om_path");
}

TEST_F(UtestFileConstantUtilTransfer, SaveWeightToOneFile_AllExist_CovEnhance) {
  ConstNodeWeightHashList list;
  auto graph = std::make_shared<ComputeGraph>("test_graph");
  auto op_desc = std::make_shared<OpDesc>("const1", "Const");
  auto node = graph->AddNode(op_desc);
  GeTensorPtr tensor = std::make_shared<GeTensor>();
  std::vector<uint8_t> data(4, 1);
  tensor->SetData(data);
  list.emplace_back(node, std::make_pair(tensor, "hash1"));

  FileConstantMeta meta;
  meta.hash_to_weight_file["hash1"] = "/tmp/weight1";

  std::string weight_dir = "./test_all_exist_dir";
  ASSERT_TRUE(CreateDirectory(weight_dir) == 0);
  auto ret = FileConstantUtils::SaveWeightToOneFileWithReuse(list, weight_dir, meta);
  EXPECT_EQ(ret, SUCCESS);
  (void)mmRmdir(weight_dir.c_str());
}

TEST_F(UtestFileConstantUtilTransfer, SaveWeightToOneFile_OpenFail_CovEnhance) {
  ConstNodeWeightHashList list;
  auto graph = std::make_shared<ComputeGraph>("test_graph");
  auto op_desc = std::make_shared<OpDesc>("const1", "Const");
  auto node = graph->AddNode(op_desc);
  GeTensorPtr tensor = std::make_shared<GeTensor>();
  std::vector<uint8_t> data(4, 1);
  tensor->SetData(data);
  list.emplace_back(node, std::make_pair(tensor, "hash1"));

  FileConstantMeta meta;
  std::string weight_dir = "/nonexistent_dir/no_such_path";
  auto ret = FileConstantUtils::SaveWeightToOneFileWithReuse(list, weight_dir, meta);
  EXPECT_EQ(ret, FAILED);
}

TEST_F(UtestFileConstantUtilTransfer, WriteWeightWithPadding_WriteFail_CovEnhance) {
  std::ofstream ofs;
  uint8_t data[] = {1, 2, 3, 4};
  size_t offset = 0;
  auto ret = FileConstantUtils::WriteWeightWithPadding(ofs, data, sizeof(data), offset);
  EXPECT_EQ(ret, FAILED);
}

TEST_F(UtestFileConstantUtilTransfer, WriteWeightWithPadding_WithPadding_CovEnhance) {
  std::string file_name = "test_padding_write.bin";
  std::ofstream ofs(file_name, std::ios::binary);
  ASSERT_TRUE(ofs.is_open());
  uint8_t data[] = {1, 2, 3, 4};
  size_t offset = 0;
  auto ret = FileConstantUtils::WriteWeightWithPadding(ofs, data, sizeof(data), offset);
  EXPECT_EQ(ret, SUCCESS);
  EXPECT_EQ(offset, 512U);
  ofs.close();
  (void)remove(file_name.c_str());
}

TEST_F(UtestFileConstantUtilTransfer, WriteWeightWithPadding_OffsetOverflow_CovEnhance) {
  std::string file_name = "test_overflow_write.bin";
  std::ofstream ofs(file_name, std::ios::binary);
  ASSERT_TRUE(ofs.is_open());
  uint8_t data[] = {1, 2, 3, 4};
  size_t offset = std::numeric_limits<size_t>::max() - 10;
  auto ret = FileConstantUtils::WriteWeightWithPadding(ofs, data, sizeof(data), offset);
  EXPECT_EQ(ret, FAILED);
  ofs.close();
  (void)remove(file_name.c_str());
}

TEST_F(UtestFileConstantUtilTransfer, ChangeFilePathAttr_NoTmpDir_CovEnhance) {
  std::string file_name = "test_no_tmp_dir_file.bin";
  std::ofstream ofs(file_name, std::ios::binary);
  ofs << "test";
  ofs.close();

  auto builder = ut::GraphBuilder("graph_no_tmp");
  auto file_const = builder.AddNode("fc_notmp", FILECONSTANT, 0, 1, FORMAT_ND, DT_FLOAT, {3});
  OpDescPtr op_desc = file_const->GetOpDesc();
  EXPECT_TRUE(AttrUtils::SetStr(op_desc, ATTR_NAME_LOCATION, file_name));

  auto netoutput = builder.AddNode("out_notmp", "NetOutput", 1, 0);
  builder.AddDataEdge(file_const, 0, netoutput, 0);
  auto graph = builder.GetGraph();

  std::string om_path = "om_path_no_tmp/test.om";
  auto ret = FileConstantUtils::ChangeFilePath(graph, om_path);
  EXPECT_EQ(ret, SUCCESS);
  (void)remove(file_name.c_str());
  (void)mmRmdir("om_path_no_tmp");
}

TEST_F(UtestFileConstantUtilTransfer, ReadJsonFile_InvalidJson_CovEnhance) {
  ExternalWeightManagerPool::Instance().Destroy();
  GetContext().SetSessionId(0U);

  std::string weight_dir = "./test_invalid_json/weight";
  ASSERT_TRUE(CreateDirectory(weight_dir) == 0);
  std::string meta_path = weight_dir + "/meta.json";
  std::ofstream meta_ofs(meta_path);
  meta_ofs << "{invalid json content";
  meta_ofs.close();

  const auto &external_weight_manager = ExternalWeightManagerPool::Instance().GetManager(GetContext().SessionId());
  ASSERT_NE(external_weight_manager, nullptr);
  external_weight_manager->SetWeightPath(weight_dir);

  ge::ut::GraphBuilder builder("graph_invalid_json");
  auto const1 = builder.AddNode("const_invalid", "Const", 0, 1);
  ge::GeTensorPtr tensor = std::make_shared<GeTensor>();
  std::vector<uint8_t> value(4 * sizeof(float));
  std::vector<int64_t> shape{4};
  tensor->MutableTensorDesc().SetShape(GeShape(shape));
  tensor->SetData(value);
  tensor->MutableTensorDesc().SetDataType(DT_FLOAT);
  ConstantUtils::SetWeight(const1->GetOpDesc(), 0, tensor);
  AttrUtils::SetStr(const1->GetOpDesc(), ATTR_NAME_WEIGHT_SHA256, "hash_invalid");

  auto ret = FileConstantUtils::ConvertConstToFileConst(builder.GetGraph());
  EXPECT_NE(ret, SUCCESS);

  (void)remove(meta_path.c_str());
  (void)mmRmdir(weight_dir.c_str());
  (void)mmRmdir("./test_invalid_json");
  ExternalWeightManagerPool::Instance().Destroy();
}
}  // namespace fileconstant
}  // namespace ge
