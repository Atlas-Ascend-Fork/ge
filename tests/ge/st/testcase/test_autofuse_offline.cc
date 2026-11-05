/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "jit_execution/user_graph_ctrl.h"
#include "hybrid/model/hybrid_model_builder.h"
#include <stdio.h>
#include <gtest/gtest.h>
#include "mmpa/mmpa_api.h"
#include "macro_utils/dt_public_scope.h"
#include "framework/common/helper/model_helper.h"
#include "common/helper/model_parser_base.h"
#include "hybrid/node_executor/aicore/aicore_op_task.h"
#include "graph/utils/graph_utils_ex.h"
#include "graph/utils/graph_utils.h"
#include "stub/gert_runtime_stub.h"
#include "common/share_graph.h"
#include "proto/task.pb.h"
#include "faker/ge_model_builder.h"
#include "common/opskernel/ops_kernel_info_types.h"
#include "common/env_path.h"
#include <filesystem>
#include "common/op_so_store/op_so_store.h"
#include "api/aclgrph/option_utils.h"
#include "engine/aicore/autofuse_dispatcher/dispatcher_table.h"
#include "lowering/model_converter.h"
#include "common/model/ge_root_model.h"
#include "common/file_constant_utils/file_constant_utils.h"
#include "faker/model_data_faker.h"
#include "faker/aicore_taskdef_faker.h"
#include "framework/runtime/gert_api.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/passes/graph_builder_utils.h"

namespace ge {
const char *const kEnvName = "ASCEND_OPP_PATH";
const char *const kEnvNameCustom = "ASCEND_CUSTOM_OPP_PATH";
const string kOpsProto = "libopsproto_rt2.0.so";
const string kOpMaster = "libopmaster_rt2.0.so";
const string kInner = "built-in";
const string kOpsProtoPath = "/op_proto/lib/linux/x86_64/";
const string kOpMasterPath = "/op_impl/ai_core/tbe/op_tiling/lib/linux/x86_64/";
#if 0
ge-dev失败，ge仓正常，两个仓合并时，再打开
static GeRootModelPtr ConstructGeRootModel(bool is_dynamic_shape = true,
                                           ModelTaskType task_type = ModelTaskType::MODEL_TASK_KERNEL,
                                           ccKernelType kernel_type = ccKernelType::TE,
                                           std::string so_name = "") {
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("graph");
  AttrUtils::SetBool(graph, ATTR_NAME_DYNAMIC_SHAPE_PARTITIONED, is_dynamic_shape);
  GeRootModelPtr ge_root_model = std::make_shared<GeRootModel>();
  EXPECT_EQ(ge_root_model->Initialize(graph), SUCCESS);
  GeModelPtr ge_model = std::make_shared<GeModel>();
  ge_root_model->subgraph_instance_name_to_model_["graph"] = ge_model;
  ge_model->SetGraph(graph);
  (void)ge::AttrUtils::SetStr(ge_model, ATTR_MODEL_HOST_ENV_OS, "linux");
  (void)ge::AttrUtils::SetStr(ge_model, ATTR_MODEL_HOST_ENV_CPU, "x86_64");

  GeModelPtr ge_model1 = std::make_shared<GeModel>();
  ge::ComputeGraphPtr graph1 = std::make_shared<ge::ComputeGraph>("graph1");
  ge_model1->SetGraph(graph1);
  ge_root_model->subgraph_instance_name_to_model_["graph1"] = ge_model1;

  {
    domi::ModelTaskDef model_task_def;
    std::shared_ptr<domi::ModelTaskDef> model_task_def_ptr = make_shared<domi::ModelTaskDef>(model_task_def);
    domi::TaskDef *task_def = model_task_def_ptr->add_task();
    ge_model->SetModelTaskDef(model_task_def_ptr);
    ge_model1->SetModelTaskDef(model_task_def_ptr);

    auto aicore_task = std::unique_ptr<hybrid::AiCoreOpTask>(new(std::nothrow)hybrid::AiCoreOpTask());
    task_def->set_type(static_cast<uint32_t>(task_type));

    domi::KernelDef *kernel_def = task_def->mutable_kernel();
    kernel_def->set_block_dim(32);
    kernel_def->set_args_size(64);
    kernel_def->set_so_name(so_name);
    string args(64, '1');
    uint16_t args_offset[9] = {0};
    kernel_def->set_args(args.data(), 64);
    domi::KernelContext *context = kernel_def->mutable_context();
    context->set_kernel_type(static_cast<uint32_t>(kernel_type));
    context->set_op_index(1);
    context->set_args_offset(args_offset, 9 * sizeof(uint16_t));
  }
  return ge_root_model;
}
#endif
std::string GetAutofuseSoPath() {
  std::string cmake_binary_path = CMAKE_BINARY_DIR;
  return cmake_binary_path + "/tests/depends/op_stub/libautofuse_stub.so";
}

ge::ComputeGraphPtr BuildAutofuseGraph() {
  auto graph = gert::ShareGraph::AutoFuseNodeGraph();
  (void)ge::AttrUtils::SetInt(graph, "_all_symbol_num", 8);
  auto fused_graph_node = graph->FindNode("fused_graph");
  auto fused_graph_node1 = graph->FindNode("fused_graph1");

  auto autofuse_stub_so = GetAutofuseSoPath();
  std::cout << "bin path: " << autofuse_stub_so << std::endl;
  (void)ge::AttrUtils::SetStr(fused_graph_node->GetOpDesc(), "bin_file_path", autofuse_stub_so);
  (void)ge::AttrUtils::SetStr(fused_graph_node1->GetOpDesc(), "bin_file_path", autofuse_stub_so);

  return graph;
}

bool BuildSoBinsPayloadForAutofuseSt(std::vector<uint8_t> &payload) {
  auto so_data_buf = std::unique_ptr<char_t[]>(new (std::nothrow) char_t[3U]);
  if (so_data_buf == nullptr) {
    return false;
  }
  so_data_buf[0] = static_cast<char_t>(0x11);
  so_data_buf[1] = static_cast<char_t>(0x22);
  so_data_buf[2] = static_cast<char_t>(0x33);

  const auto so_bin =
      std::make_shared<OpSoBin>("libautofuse_stub.so", "vendor_ut", std::move(so_data_buf), 3U, SoBinType::kAutofuse);
  if (so_bin == nullptr) {
    return false;
  }

  OpSoStore op_so_store;
  op_so_store.AddKernel(so_bin);
  if (!op_so_store.Build()) {
    return false;
  }

  const auto *data = op_so_store.Data();
  const auto size = op_so_store.DataSize();
  if ((data == nullptr) || (size == 0U)) {
    return false;
  }
  payload.assign(data, data + size);
  return true;
}

ComputeGraphPtr BuildFileConstantGraphSt(const std::string &graph_name, const std::string &shared_path,
                                         const std::string &extra_path) {
  ut::GraphBuilder builder(graph_name);
  auto shared_fc = builder.AddNode("shared_fc", FILECONSTANT, 0, 1, FORMAT_ND, DT_FLOAT, {3});
  if (extra_path.empty()) {
    auto output = builder.AddNode("output", "NetOutPut", 1, 0);
    builder.AddDataEdge(shared_fc, 0, output, 0);
  } else {
    auto extra_fc = builder.AddNode("extra_fc", FILECONSTANT, 0, 1, FORMAT_ND, DT_FLOAT, {3});
    auto output = builder.AddNode("output", "NetOutPut", 2, 0);
    builder.AddDataEdge(shared_fc, 0, output, 0);
    builder.AddDataEdge(extra_fc, 0, output, 1);
    (void)AttrUtils::SetStr(extra_fc->GetOpDesc(), ATTR_NAME_LOCATION, extra_path);
    (void)AttrUtils::SetDataType(extra_fc->GetOpDesc(), "dtype", DT_FLOAT);
  }
  (void)AttrUtils::SetStr(shared_fc->GetOpDesc(), ATTR_NAME_LOCATION, shared_path);
  (void)AttrUtils::SetDataType(shared_fc->GetOpDesc(), "dtype", DT_FLOAT);
  return builder.GetGraph();
}

std::string GetHostArchSt() {
#if defined(__aarch64__) || defined(__arm64__)
  return "aarch64";
#else
  return "x86_64";
#endif
}

struct DualBodyStModels {
  GeRootModelPtr main_root;
  GeRootModelPtr fallback_root;
  std::string main_graph_name;
};

GeModelPtr BuildSubModelSt(const std::string &graph_name) {
  auto graph = BuildAutofuseGraph();
  graph->SetName(graph_name);
  auto root = gert::GeModelBuilder(graph).BuildGeRootModel();
  if (root == nullptr) {
    return nullptr;
  }
  const auto &models = root->GetSubgraphInstanceNameToModel();
  return models.empty() ? nullptr : models.begin()->second;
}

DualBodyStModels BuildDualBodyStModels(bool with_submodel, bool with_file_constant, bool diff_weight) {
  DualBodyStModels result;
  auto main_graph = BuildAutofuseGraph();
  result.main_graph_name = main_graph->GetName();
  result.main_root = gert::GeModelBuilder(main_graph).BuildGeRootModel();
  if (result.main_root == nullptr) {
    return result;
  }
  auto fallback_graph = std::make_shared<ComputeGraph>(kFallbackModelKey);
  if (GraphUtils::CopyComputeGraph(result.main_root->GetRootGraph(), fallback_graph) != SUCCESS) {
    return result;
  }
  if (with_file_constant) {
    (void)fallback_graph->AddNode(std::make_shared<OpDesc>("fallback_file_constant", FILECONSTANT));
  }
  result.fallback_root = gert::GeModelBuilder(fallback_graph).BuildGeRootModel();
  if (result.fallback_root == nullptr) {
    return result;
  }
  if (with_submodel) {
    auto main_sub_model = BuildSubModelSt("sub_graph");
    auto fallback_sub_model = BuildSubModelSt(std::string(kFallbackSubgraphPrefix) + "sub_graph");
    if ((main_sub_model == nullptr) || (fallback_sub_model == nullptr)) {
      result.main_root = nullptr;
      return result;
    }
    result.main_root->SetSubgraphInstanceNameToModel("sub_graph", main_sub_model);
    result.fallback_root->SetSubgraphInstanceNameToModel(std::string(kFallbackSubgraphPrefix) + "sub_graph",
                                                         fallback_sub_model);
  }
  if (diff_weight) {
    const auto &models = result.fallback_root->GetSubgraphInstanceNameToModel();
    const auto iter = models.find(kFallbackModelKey);
    if (iter != models.end()) {
      const std::vector<uint8_t> weight{1U, 2U, 3U, 4U};
      iter->second->SetWeight(Buffer::CopyFrom(weight.data(), weight.size()));
    }
  }
  return result;
}

bool SaveAndLoadDualBodySt(const DualBodyStModels &models, const std::string &case_name, GeRootModelPtr &loaded) {
  const std::string om_path = EnvPath().GetOrCreateCaseTmpPath(case_name) + "/dual_body.om";
  ModelHelper save_helper;
  save_helper.SetSaveMode(true);
  ModelBufferData model_buff;
  if (save_helper.SaveDualBodyRootModel(models.main_root, models.fallback_root, om_path, model_buff, true) != SUCCESS) {
    return false;
  }
  const std::string arch =
#if defined(__aarch64__) || defined(__arm64__)
      "aarch64";
#else
      "x86_64";
#endif
  std::string actual_om_path = om_path;
  const auto dot_pos = actual_om_path.find(".om");
  if (dot_pos == std::string::npos) {
    return false;
  }
  actual_om_path.insert(dot_pos, "_linux_" + arch);
  ModelData model_data;
  if (ModelParserBase::LoadFromFile(actual_om_path.c_str(), -1, model_data) != SUCCESS) {
    return false;
  }
  ModelHelper load_helper;
  load_helper.SetRepackSoFlag(true);
  const auto ret = load_helper.LoadRootModel(model_data);
  if (model_data.model_data != nullptr) {
    delete[] static_cast<char_t *>(model_data.model_data);
    model_data.model_data = nullptr;
  }
  loaded = load_helper.GetGeRootModel();
  return (ret == SUCCESS) && (loaded != nullptr);
}

class AutofuseOfflineSt : public testing::Test {
 protected:
  void SetUp() override {
    old_global_options_ = GetThreadLocalContext().GetAllGlobalOptions();
    old_graph_options_ = GetThreadLocalContext().GetAllGraphOptions();
    const char *env_value = std::getenv("AUTOFUSE_FLAGS");
    has_autofuse_env_ = (env_value != nullptr);
    if (has_autofuse_env_) {
      autofuse_env_ = env_value;
    }
    const char *old_opp_path = std::getenv("ASCEND_OPP_PATH");
    has_opp_env_ = (old_opp_path != nullptr);
    if (has_opp_env_) {
      opp_env_ = old_opp_path;
    }
    const std::string opp_root = EnvPath().GetOrCreateCaseTmpPath("autofuse_st_opp");
    const std::string arch = GetHostArchSt();
    const std::string proto_dir = opp_root + "/built-in/op_proto/lib/linux/" + arch;
    const std::string tiling_dir = opp_root + "/built-in/op_impl/ai_core/tbe/op_tiling/lib/linux/" + arch;
    std::filesystem::create_directories(proto_dir);
    std::filesystem::create_directories(tiling_dir);
    const std::vector<std::string> so_files = {proto_dir + "/libopsproto_rt2.0.so",
                                               tiling_dir + "/libopmaster_rt2.0.so"};
    for (const auto &so_file : so_files) {
      std::ofstream ofs(so_file, std::ios::binary);
      ofs << "fake opp so";
    }
    (void)setenv("ASCEND_OPP_PATH", opp_root.c_str(), 1);
  }

  void TearDown() override {
    (void)GetThreadLocalContext().SetGlobalOption(old_global_options_);
    (void)GetThreadLocalContext().SetGraphOption(old_graph_options_);
    if (has_autofuse_env_) {
      (void)setenv("AUTOFUSE_FLAGS", autofuse_env_.c_str(), 1);
    } else {
      (void)unsetenv("AUTOFUSE_FLAGS");
    }
    if (has_opp_env_) {
      (void)setenv("ASCEND_OPP_PATH", opp_env_.c_str(), 1);
    } else {
      (void)unsetenv("ASCEND_OPP_PATH");
    }
  }

 private:
  std::map<std::string, std::string> old_global_options_;
  std::map<std::string, std::string> old_graph_options_;
  std::string autofuse_env_;
  std::string opp_env_;
  bool has_autofuse_env_ = false;
  bool has_opp_env_ = false;
};
TEST_F(AutofuseOfflineSt, CheckSaveAutofuseSo) {
  std::string output_file{"split_opp.om"};
  auto graph = BuildAutofuseGraph();
  auto ge_root_model = gert::GeModelBuilder(graph).BuildGeRootModel();

  ModelHelper model_helper;
  ModelBufferData model_buff;
  Status ret = model_helper.SaveToOmRootModel(ge_root_model, output_file, model_buff, true);
  EXPECT_EQ(ret, SUCCESS);

  OmFileLoadHelper load_helper;
  load_helper.is_inited_ = true;
  OmFileContext cur_ctx;
  ModelPartition so_patition;
  so_patition.type = ModelPartitionType::SO_BINS;
  std::vector<uint8_t> so_payload;
  ASSERT_TRUE(BuildSoBinsPayloadForAutofuseSt(so_payload));
  so_patition.data = so_payload.data();
  so_patition.size = so_payload.size();
  cur_ctx.partition_datas_.push_back(so_patition);
  load_helper.model_contexts_.push_back(cur_ctx);
  std::vector<OpSoBinPtr> custom_op_so_bins;
  EXPECT_EQ(model_helper.LoadOpSoBin(load_helper, ge_root_model, custom_op_so_bins), SUCCESS);
}

TEST_F(AutofuseOfflineSt, CheckDualBodySaveLoadForDynamicShape) {
  auto main_graph = BuildAutofuseGraph();
  auto main_root_model = gert::GeModelBuilder(main_graph).BuildGeRootModel();
  ASSERT_NE(main_root_model, nullptr);

  auto fallback_graph = std::make_shared<ComputeGraph>(kFallbackModelKey);
  ASSERT_EQ(GraphUtils::CopyComputeGraph(main_root_model->GetRootGraph(), fallback_graph), SUCCESS);
  auto fallback_root_model = gert::GeModelBuilder(fallback_graph).BuildGeRootModel();
  ASSERT_NE(fallback_root_model, nullptr);

  const std::string arch =
#if defined(__aarch64__) || defined(__arm64__)
      "aarch64";
#else
      "x86_64";
#endif
  (void)GetThreadLocalContext().SetGlobalOption({{"ge.host_env_os", "linux"}, {"ge.host_env_cpu", arch}});

  const std::string om_path = EnvPath().GetOrCreateCaseTmpPath("autofuse_dual_body") + "/dual_body.om";
  ModelHelper save_helper;
  save_helper.SetSaveMode(true);
  ModelBufferData model_buff;
  ASSERT_EQ(save_helper.SaveDualBodyRootModel(main_root_model, fallback_root_model, om_path, model_buff, true),
            SUCCESS);
  std::string actual_om_path = om_path;
  const auto dot_pos = actual_om_path.find(".om");
  ASSERT_NE(dot_pos, std::string::npos);
  actual_om_path.insert(dot_pos, "_linux_" + arch);

  ModelData model_data;
  ASSERT_EQ(ModelParserBase::LoadFromFile(actual_om_path.c_str(), -1, model_data), SUCCESS);
  ModelHelper load_helper;
  load_helper.SetRepackSoFlag(true);
  ASSERT_EQ(load_helper.LoadRootModel(model_data), SUCCESS);
  if (model_data.model_data != nullptr) {
    delete[] static_cast<char_t *>(model_data.model_data);
    model_data.model_data = nullptr;
  }
  auto loaded_root_model = load_helper.GetGeRootModel();
  ASSERT_NE(loaded_root_model, nullptr);
  const auto &name_to_model = loaded_root_model->GetSubgraphInstanceNameToModel();
  EXPECT_NE(name_to_model.find(main_graph->GetName()), name_to_model.end());
  EXPECT_NE(name_to_model.find(kFallbackModelKey), name_to_model.end());
  ASSERT_NE(name_to_model.at(kFallbackModelKey), nullptr);
  ASSERT_NE(name_to_model.at(kFallbackModelKey)->GetGraph(), nullptr);
  EXPECT_EQ(name_to_model.at(kFallbackModelKey)->GetGraph()->GetName(), kFallbackModelKey);
  bool dual_body = false;
  EXPECT_TRUE(AttrUtils::GetBool(loaded_root_model->GetRootGraph(), kDualBodyAttr, dual_body));
  EXPECT_TRUE(dual_body);
}

// ge-dev失败，ge仓正常，两个仓合并时，再打开。
// TEST_F(AutofuseOfflineSt, CheckLoadAutofuseSo) {
//   std::string ut_run_base = EnvPath().GetOrCreateCaseTmpPath("autofuse");
//   auto opp_path = ut_run_base + "/opp/";
//   mmSetEnv(kEnvName, opp_path.c_str(), 1);
//
//   std::string inner_op_master = opp_path + "built_in/op_master_device/lib/";
//   system(("mkdir -p " + inner_op_master).c_str());
//   inner_op_master += "Ascend-V7.6-libopmaster.so";
//   system(("touch " + inner_op_master).c_str());
//   system(("echo 'Ascend-V7.6-libopmaster' > " + inner_op_master).c_str());
//
//   std::string inner_proto_path = opp_path + kInner + kOpsProtoPath;
//   system(("mkdir -p " + inner_proto_path).c_str());
//   inner_proto_path += kOpsProto;
//   system(("touch " + inner_proto_path).c_str());
//   system(("echo 'ops proto' > " + inner_proto_path).c_str());
//
//   std::string inner_tiling_path = opp_path + kInner + kOpMasterPath;
//   system(("mkdir -p " + inner_tiling_path).c_str());
//   inner_tiling_path += kOpMaster;
//   system(("touch " + inner_tiling_path).c_str());
//   system(("echo 'op tiling ' > " + inner_tiling_path).c_str());
//
//   ModelBufferData model;
//   ModelHelper model_helper;
//   model_helper.SetSaveMode(true);
//   GeRootModelPtr ge_root_model =
//       ConstructGeRootModel(true, ModelTaskType::MODEL_TASK_PREPROCESS_KERNEL, ccKernelType::AI_CPU, inner_op_master);
//   EXPECT_EQ(ge_root_model->CheckAndSetNeedSoInOM(), SUCCESS);
//   EXPECT_EQ(ge_root_model->GetSoInOmFlag(), 0xc000);
//
//   auto ge_root_graph = ge_root_model->GetRootGraph();
//
//   OpDescBuilder op_desc_builder("test", "AscBackend");
//   const auto &op_desc = op_desc_builder.Build();
//   auto node = ge_root_graph->AddNode(op_desc);
//   node->SetOwnerComputeGraph(ge_root_graph);
//   auto autofuse_stub_so = inner_op_master;
//   std::cout << "bin path: " << autofuse_stub_so << std::endl;
//   auto nodes = ge_root_graph->GetAllNodesPtr();
//   for (auto n : nodes) {
//     (void)ge::AttrUtils::SetStr(n->GetOpDesc(), "bin_file_path", autofuse_stub_so);
//   }
//
//   EXPECT_EQ(ge_root_model->CheckAndSetNeedSoInOM(), SUCCESS);
//
//   std::map<std::string, std::string> options_map;
//   options_map["ge.host_env_os"] = "linux";
//   options_map["ge.host_env_cpu"] = "x86_64";
//   GetThreadLocalContext().SetGlobalOption(options_map);
//   std::string output_file = opp_path + "/output.om";
//   EXPECT_EQ(model_helper.SaveToOmRootModel(ge_root_model, output_file, model, false), SUCCESS);
//
//   ge::ModelParserBase base;
//   ge::ModelData model_data;
//   EXPECT_EQ(base.LoadFromFile((opp_path + "output_linux_x86_64.om").c_str(), -1, model_data), SUCCESS);
//   EXPECT_EQ(model_helper.LoadRootModel(model_data), SUCCESS);
//   if (model_data.model_data != nullptr) {
//     delete[] reinterpret_cast<char_t *>(model_data.model_data);
//   }
//
//   const auto &root_model = model_helper.GetGeRootModel();
//   const auto &so_list = root_model->GetAllSoBin();
//   EXPECT_EQ(so_list.size(), 4UL);
//   unordered_map<SoBinType, uint32_t> res;
//   res.emplace(SoBinType::kSpaceRegistry, 0U);
//   res.emplace(SoBinType::kOpMasterDevice, 0U);
//   std::for_each(so_list.begin(), so_list.end(), [&res](const OpSoBinPtr &so_bin) { res[so_bin->GetSoBinType()]++; });
//   EXPECT_EQ(res[SoBinType::kSpaceRegistry], 2U);
//   EXPECT_EQ(res[SoBinType::kOpMasterDevice], 1U);
//   system(("rm -rf " + opp_path).c_str());
// }

TEST_F(AutofuseOfflineSt, CheckDualBodySaveLoadWithSubgraphAndDifferentWeights) {
  const auto models = BuildDualBodyStModels(true, false, true);
  ASSERT_NE(models.main_root, nullptr);
  ASSERT_NE(models.fallback_root, nullptr);
  const std::string arch =
#if defined(__aarch64__) || defined(__arm64__)
      "aarch64";
#else
      "x86_64";
#endif
  (void)GetThreadLocalContext().SetGlobalOption({{"ge.host_env_os", "linux"}, {"ge.host_env_cpu", arch}});
  GeRootModelPtr loaded = nullptr;
  ASSERT_TRUE(SaveAndLoadDualBodySt(models, "autofuse_dual_body_sub", loaded));
  const auto &name_to_model = loaded->GetSubgraphInstanceNameToModel();
  EXPECT_NE(name_to_model.find(models.main_graph_name), name_to_model.end());
  EXPECT_NE(name_to_model.find(kFallbackModelKey), name_to_model.end());
  EXPECT_NE(name_to_model.find(std::string(kFallbackSubgraphPrefix) + "sub_graph"), name_to_model.end());
  bool weights_identical = true;
  (void)AttrUtils::GetBool(loaded->GetRootGraph(), kWeightsIdenticalAttr, weights_identical);
  EXPECT_FALSE(weights_identical);
}

TEST_F(AutofuseOfflineSt, CheckDualBodySaveLoadWithFileConstantFallback) {
  const auto models = BuildDualBodyStModels(false, true, false);
  ASSERT_NE(models.main_root, nullptr);
  ASSERT_NE(models.fallback_root, nullptr);
  const std::string arch =
#if defined(__aarch64__) || defined(__arm64__)
      "aarch64";
#else
      "x86_64";
#endif
  (void)GetThreadLocalContext().SetGlobalOption({{"ge.host_env_os", "linux"}, {"ge.host_env_cpu", arch}});
  GeRootModelPtr loaded = nullptr;
  ASSERT_TRUE(SaveAndLoadDualBodySt(models, "autofuse_dual_body_fc", loaded));
  bool weights_identical = true;
  (void)AttrUtils::GetBool(loaded->GetRootGraph(), kWeightsIdenticalAttr, weights_identical);
  EXPECT_FALSE(weights_identical);
}

TEST_F(AutofuseOfflineSt, CheckAutofuseSwitchByGraphAttr) {
  const char *env_name = "AUTOFUSE_FLAGS";
  const char *old_value = std::getenv(env_name);
  const std::string backup = (old_value == nullptr) ? "" : old_value;
  (void)unsetenv(env_name);
  EXPECT_FALSE(IsAutofuseEnabled(nullptr));
  (void)setenv(env_name, "--enable_autofuse=true", 1);
  auto graph = std::make_shared<ComputeGraph>("autofuse_switch_graph");
  EXPECT_TRUE(IsAutofuseEnabled(graph));
  (void)AttrUtils::SetBool(graph, kDisableAutofuseAttr, true);
  EXPECT_FALSE(IsAutofuseEnabled(graph));
  if (old_value != nullptr) {
    (void)setenv(env_name, backup.c_str(), 1);
  } else {
    (void)unsetenv(env_name);
  }
}

TEST_F(AutofuseOfflineSt, CheckDualBodyFileConstantChangeFilePath) {
  const std::string main_dir = "tmp_weight_st_dual_main";
  const std::string fallback_dir = "tmp_weight_st_dual_fallback";
  ASSERT_EQ(CreateDirectory(main_dir), 0);
  ASSERT_EQ(CreateDirectory(fallback_dir), 0);
  const std::string shared_path = main_dir + "/shared.bin";
  const std::string extra_path = fallback_dir + "/extra.bin";
  {
    std::ofstream shared_ofs(shared_path, std::ios::binary);
    shared_ofs << "shared";
    std::ofstream extra_ofs(extra_path, std::ios::binary);
    extra_ofs << "extra";
  }
  auto main_graph = BuildFileConstantGraphSt("main_graph", shared_path, "");
  auto fallback_graph = BuildFileConstantGraphSt(kFallbackModelKey, shared_path, extra_path);
  const std::string om_path = "st_om_dual_path/hello.om";
  ASSERT_EQ(FileConstantUtils::ChangeFilePath(main_graph, fallback_graph, om_path), SUCCESS);
  const std::string weight_dir = om_path.substr(0U, om_path.rfind('/')) + "/weight";
  EXPECT_EQ(mmAccess((weight_dir + "/shared.bin").c_str()), EN_OK);
  EXPECT_EQ(mmAccess((weight_dir + "/extra.bin").c_str()), EN_OK);
  (void)remove((weight_dir + "/shared.bin").c_str());
  (void)remove((weight_dir + "/extra.bin").c_str());
  (void)mmRmdir(weight_dir.c_str());
  (void)mmRmdir(om_path.substr(0U, om_path.rfind('/')).c_str());
  (void)mmRmdir(main_dir.c_str());
  (void)mmRmdir(fallback_dir.c_str());
}

TEST_F(AutofuseOfflineSt, CheckDualBodyWeightIdentityBranches) {
  size_t main_size = 0U;
  size_t fallback_size = 0U;
  EXPECT_FALSE(ModelHelper::AreDualBodyWeightsIdentical(nullptr, nullptr, main_size, fallback_size));
  auto main_graph = std::make_shared<ComputeGraph>("weight_main");
  auto main_root = std::make_shared<GeRootModel>();
  ASSERT_EQ(main_root->Initialize(main_graph), SUCCESS);
  auto main_model = std::make_shared<GeModel>();
  main_model->SetGraph(main_graph);
  const std::vector<uint8_t> weight{1U, 2U, 3U, 4U};
  main_model->SetWeight(Buffer::CopyFrom(weight.data(), weight.size()));
  main_root->SetSubgraphInstanceNameToModel("weight_main", main_model);

  auto fallback_graph = std::make_shared<ComputeGraph>(kFallbackModelKey);
  auto fallback_root = std::make_shared<GeRootModel>();
  ASSERT_EQ(fallback_root->Initialize(fallback_graph), SUCCESS);
  auto fallback_model = std::make_shared<GeModel>();
  fallback_model->SetGraph(fallback_graph);
  fallback_model->SetWeight(Buffer::CopyFrom(weight.data(), weight.size()));
  fallback_root->SetSubgraphInstanceNameToModel(kFallbackModelKey, fallback_model);
  EXPECT_TRUE(ModelHelper::AreDualBodyWeightsIdentical(main_root, fallback_root, main_size, fallback_size));
  EXPECT_EQ(main_size, 4U);

  const std::vector<uint8_t> diff_weight{1U, 2U, 3U, 4U, 5U, 6U};
  fallback_model->SetWeight(Buffer::CopyFrom(diff_weight.data(), diff_weight.size()));
  EXPECT_FALSE(ModelHelper::AreDualBodyWeightsIdentical(main_root, fallback_root, main_size, fallback_size));
}

class DummyModelSaveHelperSt : public ModelSaveHelper {
 public:
  Status SaveToOmRootModel(const GeRootModelPtr &, const std::string &, ModelBufferData &, const bool) override {
    return SUCCESS;
  }
  Status SaveToOmModel(const GeModelPtr &, const std::string &, ModelBufferData &, const GeRootModelPtr &) override {
    return SUCCESS;
  }
  void SetSaveMode(const bool) override {}
};

TEST_F(AutofuseOfflineSt, CheckModelSaveHelperDefaultDualBody) {
  DummyModelSaveHelperSt helper;
  ModelBufferData model;
  EXPECT_EQ(helper.SaveDualBodyRootModel(nullptr, nullptr, "dual", model, false), UNSUPPORTED);
  EXPECT_EQ(helper.ConfigureAttrCompressionMode("true"), SUCCESS);
}

TEST_F(AutofuseOfflineSt, CheckFileConstantChangeFilePathSingleGraph) {
  const std::string weight_dir = "tmp_weight_st_single";
  ASSERT_EQ(CreateDirectory(weight_dir), 0);
  const std::string weight_path = weight_dir + "/single.bin";
  {
    std::ofstream ofs(weight_path, std::ios::binary);
    ofs << "single";
  }
  auto graph = BuildFileConstantGraphSt("single_graph", weight_path, "");
  const std::string om_path = "st_om_single_path/hello.om";
  EXPECT_EQ(FileConstantUtils::ChangeFilePath(graph, om_path), SUCCESS);
  const std::string dest_dir = om_path.substr(0U, om_path.rfind('/')) + "/weight";
  EXPECT_EQ(mmAccess((dest_dir + "/single.bin").c_str()), EN_OK);
  (void)remove((dest_dir + "/single.bin").c_str());
  (void)mmRmdir(dest_dir.c_str());
  (void)mmRmdir(om_path.substr(0U, om_path.rfind('/')).c_str());
  (void)mmRmdir(weight_dir.c_str());
}

TEST_F(AutofuseOfflineSt, CheckDualBodyWeightIdentityEdgeBranches) {
  auto build_root = [](const std::string &root_name, const std::string &key, const bool with_const,
                       const bool null_model_graph) -> GeRootModelPtr {
    auto graph = std::make_shared<ComputeGraph>(root_name);
    if (with_const) {
      (void)graph->AddNode(std::make_shared<OpDesc>("const_without_weight", CONSTANT));
    }
    auto root = std::make_shared<GeRootModel>();
    if (root->Initialize(graph) != SUCCESS) {
      return nullptr;
    }
    auto model = std::make_shared<GeModel>();
    if (!null_model_graph) {
      model->SetGraph(graph);
    }
    const std::vector<uint8_t> weight{1U, 2U, 3U, 4U};
    model->SetWeight(Buffer::CopyFrom(weight.data(), weight.size()));
    root->SetSubgraphInstanceNameToModel(key, model);
    return root;
  };
  size_t main_size = 0U;
  size_t fallback_size = 0U;
  auto main_null_graph = build_root("main_graph", "main_graph", false, true);
  auto fallback_null_graph = build_root(kFallbackModelKey, kFallbackModelKey, false, true);
  EXPECT_FALSE(
      ModelHelper::AreDualBodyWeightsIdentical(main_null_graph, fallback_null_graph, main_size, fallback_size));

  auto main_const = build_root("main_graph", "main_graph", true, false);
  auto fallback_const = build_root(kFallbackModelKey, kFallbackModelKey, true, false);
  EXPECT_FALSE(ModelHelper::AreDualBodyWeightsIdentical(main_const, fallback_const, main_size, fallback_size));

  auto fallback_duplicate = build_root(kFallbackModelKey, kFallbackModelKey, false, false);
  auto dup_model = std::make_shared<GeModel>();
  dup_model->SetGraph(fallback_duplicate->GetRootGraph());
  fallback_duplicate->SetSubgraphInstanceNameToModel("__root__", dup_model);
  EXPECT_FALSE(ModelHelper::AreDualBodyWeightsIdentical(main_const, fallback_duplicate, main_size, fallback_size));
}

TEST_F(AutofuseOfflineSt, CheckDualBodySaveWithMissingRootModelKey) {
  auto build_body_root = [](const std::string &root_name, const std::string &only_key,
                            const bool include_root) -> GeRootModelPtr {
    auto graph = std::make_shared<ComputeGraph>(root_name);
    auto root = std::make_shared<GeRootModel>();
    if (root->Initialize(graph) != SUCCESS) {
      return nullptr;
    }
    auto model = std::make_shared<GeModel>();
    model->SetGraph(graph);
    if (include_root) {
      root->SetSubgraphInstanceNameToModel(root_name, model);
    }
    auto sub_graph = std::make_shared<ComputeGraph>(only_key);
    auto sub_model = std::make_shared<GeModel>();
    sub_model->SetGraph(sub_graph);
    root->SetSubgraphInstanceNameToModel(only_key, sub_model);
    return root;
  };
  ModelHelper save_helper;
  save_helper.SetSaveMode(false);
  ModelBufferData model_buff;
  auto bad_main = build_body_root("main_graph", "sub_graph", false);
  auto good_fallback = build_body_root(kFallbackModelKey, "sub_graph", true);
  EXPECT_NE(save_helper.SaveDualBodyRootModel(bad_main, good_fallback, "bad_main", model_buff, true), SUCCESS);

  auto good_main = build_body_root("main_graph", "sub_graph", true);
  auto bad_fallback = build_body_root(kFallbackModelKey, "sub_graph", false);
  ModelBufferData fallback_buff;
  EXPECT_NE(save_helper.SaveDualBodyRootModel(good_main, bad_fallback, "bad_fallback", fallback_buff, true), SUCCESS);
}

}  // namespace ge
