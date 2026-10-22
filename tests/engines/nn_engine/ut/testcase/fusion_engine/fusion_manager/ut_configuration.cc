/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "ut_configuration_fixture.h"

string ConfGetAscendPath() {
  const char *ascend_custom_path_ptr = std::getenv("ASCEND_INSTALL_PATH");
  string ascend_path;
  if (ascend_custom_path_ptr != nullptr) {
    ascend_path = fe::RealPath(string(ascend_custom_path_ptr));
  } else {
    const char *ascend_home_path_ptr = std::getenv("ASCEND_HOME");
    if (ascend_home_path_ptr != nullptr) {
      ascend_path = fe::RealPath(string(ascend_home_path_ptr));
    } else {
      ascend_path = "/mnt/d/Ascend";
    }
  }
  return ascend_path;
}

class StubForCannPath : public MmpaStubApi {
 public:
  explicit StubForCannPath(const std::string &path) : stub_path_(path) {}
  int32_t DlAddr(VOID *addr, mmDlInfo *info) override {
    info->dli_fname = stub_path_.c_str();
    info->dli_fbase = reinterpret_cast<void *>(0x1);
    info->dli_sname = "mock";
    info->dli_saddr = reinterpret_cast<void *>(0x1);
    return 0;
  }

 private:
  std::string stub_path_;
};

TEST_F(configuration_ut, init_and_finalize) {
  const std::string stub_path =
      GetCodeDir() + "/tests/engines/nn_engine/depends/CANN_910b_stub/cann/x86_64-linux/lib64/plugin";
  MmpaStub::GetInstance().SetImpl(std::make_shared<StubForCannPath>(stub_path));
  Configuration config(fe::AI_CORE_NAME);
  map<string, string> options;
  options.emplace(ge::PRECISION_MODE, ALLOW_FP32_TO_FP16);
  ge::GetThreadLocalContext().SetGraphOption(options);
  string soc_version = "Ascend910B1";
  PlatformUtils::Instance().soc_version_ = soc_version;
  Status status = config.Initialize(options);
  EXPECT_EQ(status, SUCCESS);
  status = config.Initialize(options);
  EXPECT_EQ(status, SUCCESS);
  status = config.Finalize();
  EXPECT_EQ(status, SUCCESS);
  status = config.Finalize();
  EXPECT_EQ(status, SUCCESS);
}

TEST_F(configuration_ut, get_boolvalue) {
  bool bool_value = Configuration::Instance(fe::AI_CORE_NAME).GetBoolValue("needl2fusion", false);
  ASSERT_FALSE(bool_value);
}

TEST_F(configuration_ut, get_context_string) {
  map<string, string> options;
  options.emplace("AAA", "111");
  options.emplace("BBB", "000");
  options.emplace("CCC", "");
  ge::GetThreadLocalContext().SetGraphOption(options);
  string str_value = FEContextUtils::GetGeContextValue("AAA");
  EXPECT_EQ(str_value, "111");

  str_value = FEContextUtils::GetGeContextValue("BBB");
  EXPECT_EQ(str_value, "000");

  str_value = FEContextUtils::GetGeContextValue("CCC");
  EXPECT_EQ(str_value, "");

  str_value = FEContextUtils::GetGeContextValue("DDD");
  EXPECT_EQ(str_value, "");
}

TEST_F(configuration_ut, get_boolvalue_abnormal) {
  Configuration::Instance(fe::AI_CORE_NAME).content_map_.emplace("needl2fusion2", "!@##$");
  bool bool_value = Configuration::Instance(fe::AI_CORE_NAME).GetBoolValue("needl2fusion2", false);
  ASSERT_FALSE(bool_value);
}

TEST_F(configuration_ut, loadconfigfile_success) {
  Configuration config(fe::AI_CORE_NAME);
  config.lib_path_ = GetCodeDir() + "/tests/engines/nn_engine/depends/CANN_910b_stub/cann/x86_64-linux/lib64/";
  Status status = config.LoadConfigFile();
  EXPECT_EQ(status, SUCCESS);
}

TEST_F(configuration_ut, AssembleOpsStoreInfoVector_success) {
  Configuration config(fe::AI_CORE_NAME);
  PlatformUtils::Instance().soc_version_ = "Ascend910B";
  PlatformUtils::Instance().short_soc_version_ = "Ascend910B";
  config.content_map_.clear();
  config.content_map_.emplace(
      "op.store.tbe-builtin",
      "2|6|/tests/engines/nn_engine/config/fe_config|/tests/engines/nn_engine/config/fe_config|true|true");
  config.ascend_ops_path_ = GetCurpath() + "../../../../../..";
  Status status = config.AssembleOpsStoreInfoVector();
  EXPECT_EQ(status, SUCCESS);
}

TEST_F(configuration_ut, AssembleOpsStoreInfoVector_success1) {
  Configuration &config = Configuration::Instance(fe::AI_CORE_NAME);
  PlatformUtils::Instance().soc_version_ = "Ascend310";
  PlatformUtils::Instance().short_soc_version_ = "Ascend310";
  config.content_map_.clear();
  config.content_map_.emplace(
      "op.store.tbe-builtin",
      "2|6|/tests/engines/nn_engine/config/fe_config|/tests/engines/nn_engine/config/fe_config|true|true");
  config.ascend_ops_path_ = GetCurpath() + "../../../../../..";
  Status status = config.AssembleOpsStoreInfoVector();
  EXPECT_EQ(status, SUCCESS);
  FEOpsStoreInfo op_store_info;
  config.GetOpStoreInfoByImplType(EN_IMPL_HW_TBE, op_store_info);
  string sub_path = "ascend310";
  int32_t pos = op_store_info.cfg_file_path.rfind(sub_path);
  EXPECT_EQ(pos, op_store_info.cfg_file_path.length() - sub_path.length());
}

TEST_F(configuration_ut, AssembleOpsStoreInfoVector_success3) {
  Configuration &config = Configuration::Instance(fe::AI_CORE_NAME);
  PlatformUtils::Instance().soc_version_ = "Ascend910A";
  PlatformUtils::Instance().short_soc_version_ = "Ascend910";
  config.content_map_.clear();
  config.content_map_.emplace(
      "op.store.tbe-builtin",
      "2|6|/tests/engines/nn_engine/config/fe_config|/tests/engines/nn_engine/config/fe_config|true|true");
  config.ascend_ops_path_ = GetCurpath() + "../../../../../..";
  Status status = config.AssembleOpsStoreInfoVector();
  EXPECT_EQ(status, SUCCESS);
  FEOpsStoreInfo op_store_info;
  config.GetOpStoreInfoByImplType(EN_IMPL_HW_TBE, op_store_info);
  string sub_path = "ascend910";
  int32_t pos = op_store_info.cfg_file_path.rfind(sub_path);
  EXPECT_EQ(pos, op_store_info.cfg_file_path.length() - sub_path.length());
}

TEST_F(configuration_ut, AssembleOpsStoreInfoVector_success4) {
  Configuration &config = Configuration::Instance(fe::AI_CORE_NAME);
  PlatformUtils::Instance().soc_version_ = "Ascend910B";
  PlatformUtils::Instance().short_soc_version_ = "Ascend910";
  config.content_map_.clear();
  config.content_map_.emplace(
      "op.store.tbe-builtin",
      "2|6|/tests/engines/nn_engine/config/fe_config|/tests/engines/nn_engine/config/fe_config|true|true");
  config.ascend_ops_path_ = GetCurpath() + "../../../../../..";
  Status status = config.AssembleOpsStoreInfoVector();
  EXPECT_EQ(status, SUCCESS);
  FEOpsStoreInfo op_store_info;
  config.GetOpStoreInfoByImplType(EN_IMPL_HW_TBE, op_store_info);
  string sub_path = "ascend910";
  int32_t pos = op_store_info.cfg_file_path.rfind(sub_path);
  EXPECT_EQ(pos, op_store_info.cfg_file_path.length() - sub_path.length());
}

TEST_F(configuration_ut, AssembleOpsStoreInfoVector_failed1) {
  Configuration config(fe::AI_CORE_NAME);
  map<string, string> options;
  string soc_version = "Ascend910B";
  PlatformUtils::Instance().soc_version_ = soc_version;
  options.emplace(ge::PRECISION_MODE, ALLOW_FP32_TO_FP16);
  config.Initialize(options);
  config.content_map_.clear();
  config.content_map_.emplace(
      "op.store.cce-custom",
      "0|0|./config/fe_config/cce_custom_opinfo|./config/fe_config/cce_custom_opinfo|false|false");
  config.content_map_.emplace(
      "op.store.", "1|1|./config/fe_config/tik_custom_opinfo|./config/fe_config/cce_custom_opinfo|false|false");

  Status status = config.AssembleOpsStoreInfoVector();
  EXPECT_EQ(status, OPSTORE_NAME_EMPTY);
}

TEST_F(configuration_ut, AssembleOpsStoreInfoVector_failed2) {
  Configuration config(fe::AI_CORE_NAME);
  map<string, string> options;
  string soc_version = "Ascend910B";
  PlatformUtils::Instance().soc_version_ = soc_version;
  options.emplace(ge::PRECISION_MODE, ALLOW_FP32_TO_FP16);
  config.Initialize(options);
  config.content_map_.clear();
  config.content_map_.emplace(
      "op.store.cce-custom",
      "0|0|./config/fe_config/cce_custom_opinfo|./config/fe_config/cce_custom_opinfo|false|false");
  config.content_map_.emplace("op.store.tik-custom", "");

  Status status = config.AssembleOpsStoreInfoVector();
  EXPECT_EQ(status, OPSTORE_VALUE_EMPTY);
}

TEST_F(configuration_ut, AssembleOpsStoreInfoVector_failed3) {
  Configuration config(fe::AI_CORE_NAME);
  map<string, string> options;
  string soc_version = "Ascend910B";
  PlatformUtils::Instance().soc_version_ = soc_version;
  options.emplace(ge::PRECISION_MODE, ALLOW_FP32_TO_FP16);
  config.Initialize(options);
  config.content_map_.clear();
  config.content_map_.emplace(
      "op.store.cce-custom",
      "0|0|./config/fe_config/cce_custom_opinfo|./config/fe_config/cce_custom_opinfo|false|false");
  config.content_map_.emplace("op.store.tik-custom", "0|0|./config/fe_config/cce_custom_opinfo|false|false");

  Status status = config.AssembleOpsStoreInfoVector();
  EXPECT_EQ(status, OPSTORE_VALUE_ITEM_SIZE_INCORRECT);
}

TEST_F(configuration_ut, AssembleOpsStoreInfoVector_failed4) {
  Configuration config(fe::AI_CORE_NAME);
  map<string, string> options;
  string soc_version = "Ascend910B";
  PlatformUtils::Instance().soc_version_ = soc_version;
  options.emplace(ge::PRECISION_MODE, ALLOW_FP32_TO_FP16);
  config.Initialize(options);
  config.content_map_.clear();
  config.content_map_.emplace(
      "op.store.cce-custom",
      "0|0|./config/fe_config/cce_custom_opinfo|./config/fe_config/cce_custom_opinfo|false|false");
  config.content_map_.emplace("op.store.tik-custom",
                              "1|1|./config/fe_config/tik_custom_opinfo|x|qwer|false|false|false");

  Status status = config.AssembleOpsStoreInfoVector();
  EXPECT_EQ(status, OPSTORE_VALUE_ITEM_SIZE_INCORRECT);
}

TEST_F(configuration_ut, AssembleOpsStoreInfoVector_failed5) {
  Configuration config(fe::AI_CORE_NAME);
  map<string, string> options;
  string soc_version = "Ascend910B";
  PlatformUtils::Instance().soc_version_ = soc_version;
  options.emplace(ge::PRECISION_MODE, ALLOW_FP32_TO_FP16);
  config.Initialize(options);
  config.content_map_.clear();
  config.content_map_.emplace(
      "op.store.cce-custom",
      "0|0|./config/fe_config/cce_custom_opinfo|./config/fe_config/cce_custom_opinfo|false|false");
  config.content_map_.emplace("op.store.tik-custom", "1|1|./config/fe_config/tik_custom_opinfo||false|false");

  Status status = config.AssembleOpsStoreInfoVector();
  EXPECT_EQ(status, OPSTORE_VALUE_ITEM_EMPTY);
}

TEST_F(configuration_ut, AssembleOpsStoreInfoVector_failed6) {
  Configuration config(fe::AI_CORE_NAME);
  map<string, string> options;
  string soc_version = "Ascend910B";
  PlatformUtils::Instance().soc_version_ = soc_version;
  options.emplace(ge::PRECISION_MODE, ALLOW_FP32_TO_FP16);
  config.Initialize(options);
  config.content_map_.clear();
  config.content_map_.emplace(
      "op.store.cce-custom",
      "0|0|./config/fe_config/cce_custom_opinfo|./config/fe_config/cce_custom_opinfo|false|false");
  config.content_map_.emplace("op.store.tik-custom", "1|1||./config/fe_config/tik_custom_opinfo|false|false");

  Status status = config.AssembleOpsStoreInfoVector();
  EXPECT_EQ(status, OPSTORE_VALUE_ITEM_EMPTY);
}

TEST_F(configuration_ut, AssembleOpsStoreInfoVector_failed7) {
  Configuration config(fe::AI_CORE_NAME);
  map<string, string> options;
  string soc_version = "Ascend910B";
  PlatformUtils::Instance().soc_version_ = soc_version;
  options.emplace(ge::PRECISION_MODE, ALLOW_FP32_TO_FP16);
  config.Initialize(options);
  config.content_map_.clear();

  Status status = config.AssembleOpsStoreInfoVector();
  EXPECT_EQ(status, OPSTORE_EMPTY);
}

TEST_F(configuration_ut, AssembleOpsStoreInfoVector_failed8) {
  Configuration config(fe::AI_CORE_NAME);
  map<string, string> options;
  string soc_version = "Ascend910B";
  PlatformUtils::Instance().soc_version_ = soc_version;
  options.emplace(ge::PRECISION_MODE, ALLOW_FP32_TO_FP16);
  config.Initialize(options);
  config.content_map_.clear();
  config.content_map_.emplace(
      "op.store.cce-custom",
      "0|0|./config/fe_config/cce_custom_opinfo|./config/fe_config/cce_custom_opinfo|false|false");
  config.content_map_.emplace(
      "op.store.tik-custom",
      "x|1|./config/fe_config/tik_custom_opinfo|./config/fe_config/cce_custom_opinfo|false|false");

  Status status = config.AssembleOpsStoreInfoVector();
  EXPECT_EQ(status, OPSTORE_PRIORITY_INVALID);
}

TEST_F(configuration_ut, AssembleOpsStoreInfoVector_failed9) {
  Configuration config(fe::AI_CORE_NAME);
  map<string, string> options;
  string soc_version = "Ascend910B";
  PlatformUtils::Instance().soc_version_ = soc_version;
  options.emplace(ge::PRECISION_MODE, ALLOW_FP32_TO_FP16);
  config.Initialize(options);
  config.content_map_.clear();
  config.content_map_.emplace(
      "op.store.cce-custom",
      "0|0|./config/fe_config/cce_custom_opinfo|./config/fe_config/cce_custom_opinfo|false|false");
  config.content_map_.emplace(
      "op.store.tik-custom",
      "1|c|./config/fe_config/tik_custom_opinfo|./config/fe_config/cce_custom_opinfo|false|false");

  Status status = config.AssembleOpsStoreInfoVector();
  EXPECT_EQ(status, OPSTORE_OPIMPLTYPE_INVALID);
}

TEST_F(configuration_ut, AssembleOpsStoreInfoVector_failed10) {
  Configuration config(fe::AI_CORE_NAME);
  map<string, string> options;
  string soc_version = "Ascend910B";
  PlatformUtils::Instance().soc_version_ = soc_version;
  options.emplace(ge::PRECISION_MODE, ALLOW_FP32_TO_FP16);
  config.Initialize(options);
  config.content_map_.clear();
  config.content_map_.emplace(
      "op.store.cce-custom",
      "0|0|./config/fe_config/cce_custom_opinfo|./config/fe_config/cce_custom_opinfo|false|false");
  config.content_map_.emplace(
      "op.store.tik-custom",
      "1|1099511627776|./config/fe_config/tik_custom_opinfo|./config/fe_config/cce_custom_opinfo|false|false");
  cout << "AssembleOpsStoreInfoVector_failed10 TEST:" << endl;
  Status status = config.AssembleOpsStoreInfoVector();
  EXPECT_EQ(status, OPSTORE_OPIMPLTYPE_INVALID);
}

TEST_F(configuration_ut, AssembleOpsStoreInfoVector_failed11) {
  Configuration config(fe::AI_CORE_NAME);
  map<string, string> options;
  string soc_version = "Ascend910B";
  PlatformUtils::Instance().soc_version_ = soc_version;
  options.emplace(ge::PRECISION_MODE, ALLOW_FP32_TO_FP16);
  config.Initialize(options);
  config.content_map_.clear();
  config.content_map_.emplace(
      "op.store.cce-custom",
      "0|0|./config/fe_config/cce_custom_opinfo|./config/fe_config/cce_custom_opinfo|false|false");
  config.content_map_.emplace(
      "op.store.tik-custom",
      "1|-1|./config/fe_config/tik_custom_opinfo|./config/fe_config/cce_custom_opinfo|false|false");

  Status status = config.AssembleOpsStoreInfoVector();
  EXPECT_EQ(status, OPSTORE_OPIMPLTYPE_INVALID);
}

TEST_F(configuration_ut, AssembleOpsStoreInfoVector_failed12) {
  Configuration config(fe::AI_CORE_NAME);
  map<string, string> options;
  string soc_version = "Ascend910B";
  PlatformUtils::Instance().soc_version_ = soc_version;
  options.emplace(ge::PRECISION_MODE, ALLOW_FP32_TO_FP16);
  config.Initialize(options);
  config.content_map_.clear();
  config.content_map_.emplace(
      "op.store.cce-custom",
      "0|0|./config/fe_config/cce_custom_opinfo|./config/fe_config/cce_custom_opinfo|false|false");
  config.content_map_.emplace(
      "op.store.tik-custom",
      "1|2|./config/fe_config/tik_custom_opinfo|./config/fe_config/cce_custom_opinfo|false|false");
  config.content_map_.emplace(
      "op.store.tdk-custom",
      "2|2|./config/fe_config/tik_custom_opinfo|./config/fe_config/cce_custom_opinfo|false|false");

  Status status = config.AssembleOpsStoreInfoVector();
  EXPECT_EQ(status, OPSTORE_OPIMPLTYPE_REPEAT);
}

TEST_F(configuration_ut, AssembleOpsStoreInfoVector_failed13) {
  Configuration config(fe::AI_CORE_NAME);
  map<string, string> options;
  string soc_version = "Ascend910B";
  PlatformUtils::Instance().soc_version_ = soc_version;
  options.emplace(ge::PRECISION_MODE, ALLOW_FP32_TO_FP16);
  config.Initialize(options);
  config.content_map_.clear();
  config.content_map_.emplace(
      "op.store.cce-custom",
      "0|0|./config/fe_config/cce_custom_opinfo|./config/fe_config/cce_custom_opinfo|false|false");
  config.content_map_.emplace(
      "op.store.tik-custom",
      "0|2|./config/fe_config/tik_custom_opinfo|./config/fe_config/cce_custom_opinfo|false|false");

  Status status = config.AssembleOpsStoreInfoVector();
  EXPECT_EQ(status, OPSTORE_PRIORITY_INVALID);
}

TEST_F(configuration_ut, get_opsstoreinfo) {
  Configuration::Instance(fe::AI_CORE_NAME).is_init_ = false;
  FEOpsStoreInfo tbe_builtin{
      6, "tbe-builtin", EN_IMPL_HW_TBE,
      GetCodeDir() + "/tests/engines/nn_engine/ut/testcase/fusion_engine/format_selector/fe_config/tbe_dynamic_opinfo",
      ""};
  vector<FEOpsStoreInfo> store_info;
  store_info.emplace_back(tbe_builtin);
  Configuration::Instance(fe::AI_CORE_NAME).ops_store_info_vector_ = (store_info);
  vector<FEOpsStoreInfo> ops_store_info_vec = Configuration::Instance(fe::AI_CORE_NAME).GetOpsStoreInfo();
  EXPECT_EQ(ops_store_info_vec.size(), 1);

  for (FEOpsStoreInfo &op_store_info : ops_store_info_vec) {
    if (op_store_info.op_impl_type == EN_IMPL_HW_TBE) {
      EXPECT_EQ(op_store_info.fe_ops_store_name, "tbe-builtin");
    }
    if (op_store_info.op_impl_type == EN_IMPL_VECTOR_CORE_HW_TBE) {
      EXPECT_EQ(op_store_info.fe_ops_store_name, "vectorcore-tbe-builtin");
      EXPECT_EQ(op_store_info.need_pre_compile, true);
      EXPECT_EQ(op_store_info.need_compile, false);
    }
    if (op_store_info.op_impl_type == EN_IMPL_VECTOR_CORE_CUSTOM_TBE) {
      EXPECT_EQ(op_store_info.fe_ops_store_name, "vectorcore-tbe-builtin");
      EXPECT_EQ(op_store_info.need_pre_compile, true);
      EXPECT_EQ(op_store_info.need_compile, true);
    }
    if (op_store_info.op_impl_type == EN_IMPL_PLUGIN_TBE) {
      EXPECT_EQ(op_store_info.need_pre_compile, true);
      EXPECT_EQ(op_store_info.need_compile, false);
    }
  }
}

TEST_F(configuration_ut, getstringvalue_success) {
  Configuration &config = Configuration::Instance(fe::AI_CORE_NAME);
  config.content_map_["l2cache.datavisitdist.threshold"] = "5";
  string stringvalue;
  Status status = config.GetStringValue("l2cache.datavisitdist.threshold", stringvalue);
  EXPECT_EQ(status, SUCCESS);
  EXPECT_EQ(stringvalue, "5");
}

TEST_F(configuration_ut, getstringvalue_failed) {
  string stringvalue;
  Status status = Configuration::Instance(fe::AI_CORE_NAME).GetStringValue("fusionrulemgr.xxx", stringvalue);
  EXPECT_EQ(status, FAILED);
}

TEST_F(configuration_ut, GetOpStoreInfoByImplType_success) {
  FEOpsStoreInfo tbe_builtin{
      6, "tbe-custom", EN_IMPL_CUSTOM_TBE,
      GetCodeDir() + "/tests/engines/nn_engine/ut/testcase/fusion_engine/format_selector/fe_config/tbe_dynamic_opinfo",
      ""};
  vector<FEOpsStoreInfo> store_info;
  store_info.emplace_back(tbe_builtin);
  Configuration::Instance(fe::AI_CORE_NAME).ops_store_info_vector_ = (store_info);
  FEOpsStoreInfo op_store_info;
  Status status = Configuration::Instance(fe::AI_CORE_NAME).GetOpStoreInfoByImplType(EN_IMPL_CUSTOM_TBE, op_store_info);
  EXPECT_EQ(status, SUCCESS);
  EXPECT_EQ(op_store_info.fe_ops_store_name, "tbe-custom");
}

TEST_F(configuration_ut, GetOpStoreInfoByImplType_failed) {
  FEOpsStoreInfo op_store_info;
  Status status = Configuration::Instance(fe::AI_CORE_NAME).GetOpStoreInfoByImplType(EN_RESERVED, op_store_info);
  EXPECT_EQ(status, FAILED);
}

TEST_F(configuration_ut, getgraphfilepath) {
  Configuration &config = Configuration::Instance(fe::AI_CORE_NAME);
  config.ascend_ops_path_ = GetCodeDir();
  config.content_map_["fusionrulemgr.aicore.graphfilepath"] = "built-in/fusion_rules/ai_core/built_in_graph_rules.json";
  string graph_file_path;
  Status status = config.GetGraphFilePath(graph_file_path);
  EXPECT_EQ(status, SUCCESS);
  EXPECT_NE(graph_file_path, "");
}

TEST_F(configuration_ut, getcustomfilepath) {
  Configuration &config = Configuration::Instance(fe::AI_CORE_NAME);
  config.content_map_["fusionrulemgr.aicore.customfilepath"] = "";
  string custom_file_path;
  Status status = config.GetCustomFilePath(custom_file_path);
  EXPECT_EQ(status, SUCCESS);
  EXPECT_EQ(custom_file_path, "");
}

TEST_F(configuration_ut, dsagetgraphfilepath) {
  Configuration::Instance(fe::kDsaCoreName).content_map_["fusionrulemgr.dsacore.graphfilepath"] = "";
  string graph_file_path;
  Status status = Configuration::Instance(fe::kDsaCoreName).GetGraphFilePath(graph_file_path);
  EXPECT_EQ(status, SUCCESS);
  EXPECT_EQ(graph_file_path, "");
}

TEST_F(configuration_ut, dsagetcustomfilepath) {
  Configuration::Instance(fe::kDsaCoreName).content_map_["fusionrulemgr.dsacore.customfilepath"] = "";
  string custom_file_path;
  Status status = Configuration::Instance(fe::kDsaCoreName).GetCustomFilePath(custom_file_path);
  EXPECT_EQ(status, SUCCESS);
  EXPECT_EQ(custom_file_path, "");
}

TEST_F(configuration_ut, dsagetcustompassfilepath) {
  Configuration::Instance(fe::kDsaCoreName).content_map_["fusionpassmgr.dsacore.custompasspath"] = "";
  string graph_file_path;
  Status status = Configuration::Instance(fe::kDsaCoreName).GetCustomPassFilePath(graph_file_path);
  EXPECT_EQ(status, SUCCESS);
  EXPECT_EQ(graph_file_path, "");
}

TEST_F(configuration_ut, dsagetbuiltinpassfilepath) {
  Configuration::Instance(fe::kDsaCoreName).content_map_["fusionpassmgr.dsacore.graphpasspath"] = "";
  string custom_file_path;
  Status status = Configuration::Instance(fe::kDsaCoreName).GetBuiltinPassFilePath(custom_file_path);
  EXPECT_EQ(status, SUCCESS);
  EXPECT_EQ(custom_file_path, "");
}

TEST_F(configuration_ut, get_opsstoreinfo_vectorcore) {
  Configuration config(fe::VECTOR_CORE_NAME);
  FEOpsStoreInfo tbe_builtin_vector_core(
      9, "vector-core-tbe-builtin", EN_IMPL_VECTOR_CORE_HW_TBE,
      GetCodeDir() + "/tests/engines/nn_engine/ut/testcase/fusion_engine/format_selector/fe_config/tbe_dynamic_opinfo",
      "");
  tbe_builtin_vector_core.need_pre_compile = true;
  tbe_builtin_vector_core.need_compile = true;
  vector<FEOpsStoreInfo> store_info;
  store_info.emplace_back(tbe_builtin_vector_core);
  config.ops_store_info_vector_ = (store_info);
  vector<FEOpsStoreInfo> ops_store_info_vec = config.GetOpsStoreInfo();
  EXPECT_EQ(ops_store_info_vec.size(), 1);

  for (FEOpsStoreInfo &op_store_info : ops_store_info_vec) {
    if (op_store_info.op_impl_type == EN_IMPL_VECTOR_CORE_HW_TBE) {
      EXPECT_EQ(op_store_info.fe_ops_store_name, "vector-core-tbe-builtin");
      EXPECT_EQ(op_store_info.need_pre_compile, true);
      EXPECT_EQ(op_store_info.need_compile, true);
    }
    if (op_store_info.op_impl_type == EN_IMPL_VECTOR_CORE_CUSTOM_TBE) {
      EXPECT_EQ(op_store_info.fe_ops_store_name, "vector-core-tbe-custom");
      EXPECT_EQ(op_store_info.need_pre_compile, true);
      EXPECT_EQ(op_store_info.need_compile, true);
    }
  }
}

TEST_F(configuration_ut, getgraphfilepath_vectorcore) {
  string graph_file_path;
  Configuration config(fe::VECTOR_CORE_NAME);
  string soc_version = "Ascend910B";
  PlatformUtils::Instance().soc_version_ = soc_version;
  config.lib_path_ = GetCodeDir() + "/tests/engines/nn_engine/depends/CANN_910b_stub/cann/x86_64-linux/lib64/";
  config.ascend_ops_path_ = GetCodeDir();
  config.content_map_["fusionrulemgr.vectorcore.graphfilepath"] =
      "built-in/fusion_rules/vector_core/built_in_graph_rules.json";
  Status status = config.GetGraphFilePath(graph_file_path);
  EXPECT_EQ(status, SUCCESS);
  EXPECT_NE(graph_file_path, "");
}

TEST_F(configuration_ut, getcustomfilepath_vectorcore) {
  string custom_file_path;
  Configuration config(fe::VECTOR_CORE_NAME);
  config.content_map_["fusionrulemgr.vectorcore.customfilepath"] = "";
  Status status = config.GetCustomFilePath(custom_file_path);
  EXPECT_EQ(status, SUCCESS);
  EXPECT_EQ(custom_file_path, "");
}

TEST_F(configuration_ut, init_op_precision_mode_case1) {
  Configuration config(AI_CORE_NAME);
  config.impl_mode_parser_ = std::make_shared<OpImplModeConfigParser>(config.ascend_ops_path_);
  map<string, string> options;
  // unix file format
  options.emplace("ge.exec.op_precision_mode",
                  GetCodeDir() + "/tests/engines/nn_engine/config/op_precision_mode_1.ini");
  Status status = config.impl_mode_parser_->InitializeFromOptions(options);
  EXPECT_EQ(status, SUCCESS);
  std::string str;
  EXPECT_EQ(config.GetOpImplMode("", "ccc", str), true);
  EXPECT_EQ(str, "high_performance");
  EXPECT_EQ(config.GetOpImplMode("", "ddd", str), true);
  EXPECT_EQ(str, "high_precision");
  EXPECT_EQ(config.GetOpImplMode("op_name1", "", str), true);
  EXPECT_EQ(str, "high_precision");
  EXPECT_EQ(config.GetOpImplMode("op_name2", "", str), false);

  ge::GetThreadLocalContext().SetGraphOption(options);
  status = config.impl_mode_parser_->InitializeFromContext();
  EXPECT_EQ(status, SUCCESS);
  EXPECT_EQ(config.GetOpImplMode("", "ccc", str), true);
  EXPECT_EQ(str, "high_performance");
  EXPECT_EQ(config.GetOpImplMode("", "ddd", str), true);
  EXPECT_EQ(str, "high_precision");
  EXPECT_EQ(config.GetOpImplMode("op_name1", "", str), true);
  EXPECT_EQ(str, "high_precision");
  EXPECT_EQ(config.GetOpImplMode("op_name2", "", str), false);
}

TEST_F(configuration_ut, init_op_precision_mode_case2) {
  Configuration config(AI_CORE_NAME);
  config.impl_mode_parser_ = std::make_shared<OpImplModeConfigParser>(config.ascend_ops_path_);
  map<string, string> options;
  // empty file
  options.emplace("ge.exec.op_precision_mode",
                  GetCodeDir() + "/tests/engines/nn_engine/config/op_precision_mode_2.ini");
  Status status = config.impl_mode_parser_->InitializeFromOptions(options);
  EXPECT_EQ(status, SUCCESS);
  std::string str;
  EXPECT_EQ(config.GetOpImplMode("op_name", "ccc", str), false);
  EXPECT_EQ(config.GetOpImplMode("op_name", "ddd", str), false);
}

TEST_F(configuration_ut, init_op_precision_mode_case3) {
  Configuration config(AI_CORE_NAME);
  config.impl_mode_parser_ = std::make_shared<OpImplModeConfigParser>(config.ascend_ops_path_);
  map<string, string> options;
  // dos file format
  options.emplace("ge.exec.op_precision_mode",
                  GetCodeDir() + "/tests/engines/nn_engine/config/op_precision_mode_3.ini");
  Status status = config.impl_mode_parser_->InitializeFromOptions(options);
  EXPECT_EQ(status, SUCCESS);
  std::string str;
  EXPECT_EQ(config.GetOpImplMode("", "ccc", str), true);
  EXPECT_EQ(str, "high_precision");
  EXPECT_EQ(config.GetOpImplMode("", "ddd", str), false);
  EXPECT_EQ(config.GetOpImplMode("op_name1", "", str), true);
  EXPECT_EQ(str, "high_precision");
  EXPECT_EQ(config.GetOpImplMode("op_name2", "", str), true);
  EXPECT_EQ(str, "high_performance");

  options["ge.exec.op_precision_mode"] = GetCodeDir() + "/tests/engines/nn_engine/config/op_precision_mode_1.ini";
  ge::GetThreadLocalContext().SetGraphOption(options);
  status = config.impl_mode_parser_->InitializeFromContext();
  EXPECT_EQ(status, SUCCESS);
  EXPECT_EQ(config.GetOpImplMode("", "ccc", str), true);
  EXPECT_EQ(str, "high_performance");
  EXPECT_EQ(config.GetOpImplMode("", "ddd", str), true);
  EXPECT_EQ(str, "high_precision");
  EXPECT_EQ(config.GetOpImplMode("op_name1", "", str), true);
  EXPECT_EQ(str, "high_precision");
  EXPECT_EQ(config.GetOpImplMode("op_name2", "", str), false);
}

TEST_F(configuration_ut, init_op_precision_mode_case4) {
  Configuration config(AI_CORE_NAME);
  config.impl_mode_parser_ = std::make_shared<OpImplModeConfigParser>(config.ascend_ops_path_);
  map<string, string> options;
  options["ge.exec.op_precision_mode"] = GetCodeDir() + "/tests/engines/nn_engine/config/op_precision_mode_0.ini";
  Status status = config.impl_mode_parser_->InitializeFromOptions(options);
  EXPECT_EQ(status, FAILED);

  options["ge.exec.op_precision_mode"] = GetCodeDir() + "/tests/engines/nn_engine/config/op_precision_mode_1.ini";
  ge::GetThreadLocalContext().SetGraphOption(options);
  status = config.impl_mode_parser_->InitializeFromContext();
  EXPECT_EQ(status, SUCCESS);

  status = config.impl_mode_parser_->InitializeFromOptions(options);
  EXPECT_EQ(status, SUCCESS);

  options["ge.exec.op_precision_mode"] = GetCodeDir() + "/tests/engines/nn_engine/config/op_precision_mode_0.ini";
  ge::GetThreadLocalContext().SetGraphOption(options);
  status = config.impl_mode_parser_->InitializeFromContext();
  EXPECT_EQ(status, FAILED);
}

TEST_F(configuration_ut, init_op_select_impl_mode_for_all_case1) {
  Configuration config(AI_CORE_NAME);
  config.ascend_ops_path_ = GetCodeDir();
  config.impl_mode_parser_ = std::make_shared<OpImplModeConfigParser>(config.ascend_ops_path_);
  map<string, string> options;
  options.emplace("ge.opSelectImplmode", "high_precision_for_all");
  Status status = config.impl_mode_parser_->InitializeFromOptions(options);
  EXPECT_EQ(status, SUCCESS);
  std::string str;
  EXPECT_EQ(config.GetOpImplMode("op_name", "ccc", str), false);
  EXPECT_EQ(config.GetOpImplMode("op_name", "ddd", str), false);
}

TEST_F(configuration_ut, init_op_select_impl_mode_for_all_case2) {
  Configuration config(AI_CORE_NAME);
  config.ascend_ops_path_ = GetCodeDir();
  config.impl_mode_parser_ = std::make_shared<OpImplModeConfigParser>(config.ascend_ops_path_);
  map<string, string> options;
  options.emplace("ge.opSelectImplmode", "high_performance_for_all");
  Status status = config.impl_mode_parser_->InitializeFromOptions(options);
  EXPECT_EQ(status, SUCCESS);
  std::string str;
  EXPECT_EQ(config.GetOpImplMode("op_name", "ccc", str), false);
  EXPECT_EQ(config.GetOpImplMode("op_name", "ddd", str), false);
}

TEST_F(configuration_ut, init_op_select_impl_mode_for_all_case3) {
  Configuration config(AI_CORE_NAME);
  config.ascend_ops_path_ = GetCodeDir() + "/tests/engines/nn_engine/config/";
  config.impl_mode_parser_ = std::make_shared<OpImplModeConfigParser>(config.ascend_ops_path_);
  map<string, string> options;
  options.emplace("ge.opSelectImplmode", "high_precision_for_all");
  Status status = config.impl_mode_parser_->InitializeFromOptions(options);
  EXPECT_EQ(status, SUCCESS);
  std::string str;
  EXPECT_EQ(config.GetOpImplMode("op_name", "Add", str), true);
  EXPECT_EQ(str, "high_precision");
  EXPECT_EQ(config.GetOpImplMode("op_name", "Mul", str), true);
  EXPECT_EQ(str, "high_precision");
  EXPECT_EQ(config.GetOpImplMode("op_name", "Relu", str), true);
  EXPECT_EQ(str, "high_precision");
}

TEST_F(configuration_ut, init_op_select_impl_mode_for_all_case4) {
  Configuration config(AI_CORE_NAME);
  config.ascend_ops_path_ = GetCodeDir() + "/tests/engines/nn_engine/config/";
  config.impl_mode_parser_ = std::make_shared<OpImplModeConfigParser>(config.ascend_ops_path_);
  map<string, string> options;
  options.emplace("ge.opSelectImplmode", "high_performance_for_all");
  Status status = config.impl_mode_parser_->InitializeFromOptions(options);
  EXPECT_EQ(status, SUCCESS);
  std::string str;
  EXPECT_EQ(config.GetOpImplMode("op_name", "Add", str), true);
  EXPECT_EQ(str, "high_performance");
  EXPECT_EQ(config.GetOpImplMode("op_name", "Mul", str), true);
  EXPECT_EQ(str, "high_performance");
  EXPECT_EQ(config.GetOpImplMode("op_name", "Relu", str), true);
  EXPECT_EQ(str, "high_performance");
}

TEST_F(configuration_ut, init_op_select_impl_mode_case1) {
  Configuration config(AI_CORE_NAME);
  config.ascend_ops_path_ = GetCodeDir();
  config.impl_mode_parser_ = std::make_shared<OpImplModeConfigParser>(config.ascend_ops_path_);
  map<string, string> options;
  options.emplace("ge.opSelectImplmode", "high_precision");
  Status status = config.impl_mode_parser_->InitializeFromOptions(options);
  EXPECT_EQ(status, SUCCESS);
}

TEST_F(configuration_ut, init_op_select_impl_mode_case2) {
  Configuration config(AI_CORE_NAME);
  config.ascend_ops_path_ = GetCodeDir();
  config.impl_mode_parser_ = std::make_shared<OpImplModeConfigParser>(config.ascend_ops_path_);
  map<string, string> options;
  options.emplace("ge.opSelectImplmode", "high_performance");
  Status status = config.impl_mode_parser_->InitializeFromOptions(options);
  EXPECT_EQ(status, SUCCESS);
}

TEST_F(configuration_ut, init_op_select_impl_mode_case3) {
  Configuration config(AI_CORE_NAME);
  config.ascend_ops_path_ = GetCodeDir() + "/tests/engines/nn_engine/config/";
  config.impl_mode_parser_ = std::make_shared<OpImplModeConfigParser>(config.ascend_ops_path_);
  map<string, string> options;
  options.emplace("ge.opSelectImplmode", "high_precision");
  options.emplace("ge.optypelistForImplmode", "");
  Status status = config.impl_mode_parser_->InitializeFromOptions(options);
  EXPECT_EQ(status, SUCCESS);
  std::string str;
  EXPECT_EQ(config.GetOpImplMode("op_name", "Add", str), true);
  EXPECT_EQ(str, "high_performance");
  EXPECT_EQ(config.GetOpImplMode("op_name", "Mul", str), true);
  EXPECT_EQ(str, "high_precision");
  EXPECT_EQ(config.GetOpImplMode("op_name", "Relu", str), true);
  EXPECT_EQ(str, "high_precision");
}

TEST_F(configuration_ut, init_op_select_impl_mode_case4) {
  Configuration config(AI_CORE_NAME);
  config.ascend_ops_path_ = GetCodeDir() + "/tests/engines/nn_engine/config/";
  config.impl_mode_parser_ = std::make_shared<OpImplModeConfigParser>(config.ascend_ops_path_);
  map<string, string> options;
  options.emplace("ge.opSelectImplmode", "high_performance");
  options.emplace("ge.optypelistForImplmode", "   ");
  Status status = config.impl_mode_parser_->InitializeFromOptions(options);
  EXPECT_EQ(status, SUCCESS);
  std::string str;
  EXPECT_EQ(config.GetOpImplMode("op_name", "Add", str), true);
  EXPECT_EQ(str, "high_performance");
  EXPECT_EQ(config.GetOpImplMode("op_name", "Mul", str), true);
  EXPECT_EQ(str, "high_performance");
  EXPECT_EQ(config.GetOpImplMode("op_name", "Relu", str), true);
  EXPECT_EQ(str, "high_precision");
}

TEST_F(configuration_ut, init_op_select_impl_mode_case5) {
  Configuration config(AI_CORE_NAME);
  config.ascend_ops_path_ = GetCodeDir() + "/tests/engines/nn_engine/config/";
  config.impl_mode_parser_ = std::make_shared<OpImplModeConfigParser>(config.ascend_ops_path_);
  map<string, string> options;
  options.emplace("ge.opSelectImplmode", "high_precision");
  options.emplace("ge.optypelistForImplmode", "Relu,Add,Mul");
  Status status = config.impl_mode_parser_->InitializeFromOptions(options);
  EXPECT_EQ(status, SUCCESS);
  std::string str;
  EXPECT_EQ(config.GetOpImplMode("op_name", "Add", str), true);
  EXPECT_EQ(str, "high_precision");
  EXPECT_EQ(config.GetOpImplMode("op_name", "Mul", str), true);
  EXPECT_EQ(str, "high_precision");
  EXPECT_EQ(config.GetOpImplMode("op_name", "Relu", str), true);
  EXPECT_EQ(str, "high_precision");
}

TEST_F(configuration_ut, init_op_select_impl_mode_case6) {
  Configuration config(AI_CORE_NAME);
  config.ascend_ops_path_ = GetCodeDir() + "/tests/engines/nn_engine/config/";
  config.impl_mode_parser_ = std::make_shared<OpImplModeConfigParser>(config.ascend_ops_path_);
  map<string, string> options;
  options.emplace("ge.opSelectImplmode", "high_performance");
  options.emplace("ge.optypelistForImplmode", "  Relu ,Add , Mul ");
  Status status = config.impl_mode_parser_->InitializeFromOptions(options);
  EXPECT_EQ(status, SUCCESS);
  std::string str;
  EXPECT_EQ(config.GetOpImplMode("op_name", "Add", str), true);
  EXPECT_EQ(str, "high_performance");
  EXPECT_EQ(config.GetOpImplMode("op_name", "Mul", str), true);
  EXPECT_EQ(str, "high_performance");
  EXPECT_EQ(config.GetOpImplMode("op_name", "Relu", str), true);
  EXPECT_EQ(str, "high_performance");
}

TEST_F(configuration_ut, init_allow_hf32_empty) {
  Configuration config(AI_CORE_NAME);
  PlatformUtils::Instance().pm_item_vec_[static_cast<size_t>(PlatformUtils::PlatformInfoItem::AllowHf32)] = 1;
  config.ascend_ops_path_ = GetCodeDir() + "/tests/engines/nn_engine/config/";
  config.impl_mode_parser_ = std::make_shared<OpImplModeConfigParser>(config.ascend_ops_path_);
  map<string, string> options;
  options.emplace("ge.opSelectImplmode", "high_performance");
  options.emplace("ge.optypelistForImplmode", "  Relu ,Add , Mul ");
  Status status = config.impl_mode_parser_->InitializeFromOptions(options);
  EXPECT_EQ(status, SUCCESS);
  std::string str;
  EXPECT_EQ(config.GetOpImplMode("op_name", "Add", str), true);
  EXPECT_EQ(str, "high_performance");
  EXPECT_EQ(config.GetOpImplMode("op_name", "Mul", str), true);
  EXPECT_EQ(str, "high_performance");
  EXPECT_EQ(config.GetOpImplMode("op_name", "Relu", str), true);
  EXPECT_EQ(str, "high_performance");
  EXPECT_EQ(config.GetOpImplMode("op_name", "Conv2D", str), true);
  EXPECT_EQ(str, "enable_hi_float_32_execution");
  EXPECT_EQ(config.GetOpImplMode("op_name", "MatMulV2", str), true);
  EXPECT_EQ(str, "enable_float_32_execution");
}

TEST_F(configuration_ut, init_allow_hf32_10) {
  Configuration config(AI_CORE_NAME);
  PlatformUtils::Instance().pm_item_vec_[static_cast<size_t>(PlatformUtils::PlatformInfoItem::AllowHf32)] = 1;
  config.ascend_ops_path_ = GetCodeDir() + "/tests/engines/nn_engine/config/";
  config.impl_mode_parser_ = std::make_shared<OpImplModeConfigParser>(config.ascend_ops_path_);
  map<string, string> options;
  options.emplace("ge.exec.allow_hf32", "10");
  options.emplace("ge.opSelectImplmode", "high_performance");
  options.emplace("ge.optypelistForImplmode", "  Relu ,Add , Mul ");
  Status status = config.impl_mode_parser_->InitializeFromOptions(options);
  EXPECT_EQ(status, SUCCESS);
  std::string str;
  EXPECT_EQ(config.GetOpImplMode("op_name", "Add", str), true);
  EXPECT_EQ(str, "high_performance");
  EXPECT_EQ(config.GetOpImplMode("op_name", "Mul", str), true);
  EXPECT_EQ(str, "high_performance");
  EXPECT_EQ(config.GetOpImplMode("op_name", "Relu", str), true);
  EXPECT_EQ(str, "high_performance");
  EXPECT_EQ(config.GetOpImplMode("op_name", "Conv2D", str), true);
  EXPECT_EQ(str, "enable_hi_float_32_execution");
  EXPECT_EQ(config.GetOpImplMode("op_name", "MatMulV2", str), true);
  EXPECT_EQ(str, "enable_float_32_execution");
}

TEST_F(configuration_ut, init_allow_hf32_01) {
  Configuration config(AI_CORE_NAME);
  PlatformUtils::Instance().pm_item_vec_[static_cast<size_t>(PlatformUtils::PlatformInfoItem::AllowHf32)] = 1;
  config.ascend_ops_path_ = GetCodeDir() + "/tests/engines/nn_engine/config/";
  config.impl_mode_parser_ = std::make_shared<OpImplModeConfigParser>(config.ascend_ops_path_);
  map<string, string> options;
  options.emplace("ge.exec.allow_hf32", "01");
  options.emplace("ge.opSelectImplmode", "high_performance");
  options.emplace("ge.optypelistForImplmode", "  Relu ,Add , Mul ");
  Status status = config.impl_mode_parser_->InitializeFromOptions(options);
  EXPECT_EQ(status, SUCCESS);
  std::string str;
  EXPECT_EQ(config.GetOpImplMode("op_name", "Add", str), true);
  EXPECT_EQ(str, "high_performance");
  EXPECT_EQ(config.GetOpImplMode("op_name", "Mul", str), true);
  EXPECT_EQ(str, "high_performance");
  EXPECT_EQ(config.GetOpImplMode("op_name", "Relu", str), true);
  EXPECT_EQ(str, "high_performance");
  EXPECT_EQ(config.GetOpImplMode("op_name", "Conv2D", str), true);
  EXPECT_EQ(str, "enable_float_32_execution");
  EXPECT_EQ(config.GetOpImplMode("op_name", "MatMulV2", str), true);
  EXPECT_EQ(str, "enable_hi_float_32_execution");
}

TEST_F(configuration_ut, init_allow_hf32_all_empty) {
  Configuration config(AI_CORE_NAME);
  PlatformUtils::Instance().pm_item_vec_[static_cast<size_t>(PlatformUtils::PlatformInfoItem::AllowHf32)] = 1;
  config.ascend_ops_path_ = GetCodeDir() + "/tests/engines/nn_engine/config/";
  config.impl_mode_parser_ = std::make_shared<OpImplModeConfigParser>(config.ascend_ops_path_);
  map<string, string> options;
  Status status = config.impl_mode_parser_->InitializeFromOptions(options);
  EXPECT_EQ(status, SUCCESS);
  std::string str;
  EXPECT_EQ(config.GetOpImplMode("op_name", "Conv2D", str), true);
  EXPECT_EQ(str, "enable_hi_float_32_execution");
  EXPECT_EQ(config.GetOpImplMode("op_name", "MatMulV2", str), true);
  EXPECT_EQ(str, "enable_float_32_execution");
}

TEST_F(configuration_ut, init_allow_hf32_all_invalid) {
  Configuration config(AI_CORE_NAME);
  PlatformUtils::Instance().pm_item_vec_[static_cast<size_t>(PlatformUtils::PlatformInfoItem::AllowHf32)] = 1;
  config.ascend_ops_path_ = GetCodeDir() + "/tests/engines/nn_engine/config/";
  config.impl_mode_parser_ = std::make_shared<OpImplModeConfigParser>(config.ascend_ops_path_);
  map<string, string> options;
  options.emplace("ge.exec.allow_hf32", "0111");
  Status status = config.impl_mode_parser_->InitializeFromOptions(options);
  EXPECT_EQ(status, FAILED);
}

// 回归: 用户显式 op_select_implmode 与默认 allow_hf32(10) 配置重叠时, 用户配置优先（默认值不得覆盖显式配置）
TEST_F(configuration_ut, init_user_implmode_over_default_hf32) {
  Configuration config(AI_CORE_NAME);
  PlatformUtils::Instance().pm_item_vec_[static_cast<size_t>(PlatformUtils::PlatformInfoItem::AllowHf32)] = 1;
  config.ascend_ops_path_ = GetCodeDir() + "/tests/engines/nn_engine/config/";
  config.impl_mode_parser_ = std::make_shared<OpImplModeConfigParser>(config.ascend_ops_path_);
  map<string, string> options;
  options.emplace("ge.opSelectImplmode", "high_precision");
  options.emplace("ge.optypelistForImplmode", " Conv2D , MatMulV2 ");
  Status status = config.impl_mode_parser_->InitializeFromOptions(options);
  EXPECT_EQ(status, SUCCESS);
  std::string str;
  EXPECT_EQ(config.GetOpImplMode("op_name", "Conv2D", str), true);
  EXPECT_EQ(str, "high_precision");
  EXPECT_EQ(config.GetOpImplMode("op_name", "MatMulV2", str), true);
  EXPECT_EQ(str, "high_precision");
  // 未被用户指定的算子仍回落到默认 allow_hf32 配置
  EXPECT_EQ(config.GetOpImplMode("op_name", "DepthwiseConv2D", str), true);
  EXPECT_EQ(str, "enable_hi_float_32_execution");
  EXPECT_EQ(config.GetOpImplMode("op_name", "BatchMatMul", str), true);
  EXPECT_EQ(str, "enable_float_32_execution");
}

// 显式 allow_hf32 优先级高于 op_select_implmode, 重叠算子以 allow_hf32 为准
TEST_F(configuration_ut, init_explicit_hf32_over_user_implmode) {
  Configuration config(AI_CORE_NAME);
  PlatformUtils::Instance().pm_item_vec_[static_cast<size_t>(PlatformUtils::PlatformInfoItem::AllowHf32)] = 1;
  config.ascend_ops_path_ = GetCodeDir() + "/tests/engines/nn_engine/config/";
  config.impl_mode_parser_ = std::make_shared<OpImplModeConfigParser>(config.ascend_ops_path_);
  map<string, string> options;
  options.emplace("ge.exec.allow_hf32", "10");
  options.emplace("ge.opSelectImplmode", "high_precision");
  options.emplace("ge.optypelistForImplmode", " Conv2D , MatMulV2 ");
  Status status = config.impl_mode_parser_->InitializeFromOptions(options);
  EXPECT_EQ(status, SUCCESS);
  std::string str;
  EXPECT_EQ(config.GetOpImplMode("op_name", "Conv2D", str), true);
  EXPECT_EQ(str, "enable_hi_float_32_execution");
  EXPECT_EQ(config.GetOpImplMode("op_name", "MatMulV2", str), true);
  EXPECT_EQ(str, "enable_float_32_execution");
}

TEST_F(configuration_ut, init_compress_ratio) {
  Configuration config(fe::AI_CORE_NAME);
  map<string, string> options;
  string soc_version = "Ascend910B";
  PlatformUtils::Instance().soc_version_ = soc_version;
  options.emplace(ge::PRECISION_MODE, ALLOW_FP32_TO_FP16);
  config.Initialize(options);
  config.content_map_.clear();
  config.compress_ratios_.clear();
  config.content_map_.emplace(std::make_pair("multi_core.compress_ratio", "2:0.8|8:0.8|10:0.8|30:0.8|32:0.8|48:0.8"));
  config.InitCompressRatio();
  auto compress_ratios = config.GetCompressRatios();
  for (auto compress_ratio : compress_ratios) {
    cout << "core num:" << compress_ratio.first << ", ratio:" << compress_ratio.second << endl;
  }
  EXPECT_EQ(compress_ratios.size(), 6);
  EXPECT_FLOAT_EQ(config.GetAICoreCompressRatio(), 0.8);
}

TEST_F(configuration_ut, init_compress_ratio_1) {
  Configuration config(fe::AI_CORE_NAME);
  map<string, string> options;
  string soc_version = "Ascend910B";
  PlatformUtils::Instance().soc_version_ = soc_version;
  options.emplace(ge::PRECISION_MODE, ALLOW_FP32_TO_FP16);
  options.insert(std::make_pair("ge.customizeDtypes", GetCodeDir() + "/tests/engines/nn_engine/ut/stub/custom.cfg"));
  config.Initialize(options);
  config.content_map_.clear();
  config.compress_ratios_.clear();
  config.content_map_.emplace(std::make_pair("multi_core.compress_ratio", "2:0.8:0.88|%:0.7|10:%%|30:0.8"));
  config.InitCompressRatio();
  auto compress_ratios = config.GetCompressRatios();
  for (auto compress_ratio : compress_ratios) {
    cout << "core num:" << compress_ratio.first << ", ratio:" << compress_ratio.second << endl;
  }
  EXPECT_EQ(compress_ratios.size(), 1);
  EXPECT_FLOAT_EQ(config.GetAICoreCompressRatio(), 0.8);
}
