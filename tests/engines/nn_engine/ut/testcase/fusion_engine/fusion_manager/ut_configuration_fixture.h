/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef TESTS_ENGINES_NN_ENGINE_UT_TESTCASE_FUSION_ENGINE_FUSION_MANAGER_UT_CONFIGURATION_FIXTURE_H_
#define TESTS_ENGINES_NN_ENGINE_UT_TESTCASE_FUSION_ENGINE_FUSION_MANAGER_UT_CONFIGURATION_FIXTURE_H_

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <stdlib.h>
#include <list>
#include "common/comm_error_codes.h"
#include "common/string_utils.h"
#include "fe_llt_utils.h"
#define protected public
#define private public
#include "common/platform_utils.h"
#include "common/configuration.h"
#include "common/fe_context_utils.h"
#include "common/util/json_util.h"
#include "common/util/constants.h"
#include "common/config_parser/op_impl_mode_config_parser.h"
#include "common/config_parser/op_cust_dtypes_config_parser.h"
#include "common/config_parser/modify_mixlist_config_parser.h"
#include "common/config_parser/op_debug_config_parser.h"
#include "platform/platform_info.h"
#include "mmpa/src/mmpa_stub.h"
#include "graph/ge_context.h"
#include "ge/ge_api_types.h"
#include "common/aicore_util_constants.h"
#include "fusion_config_manager/fusion_config_parser.h"
#include "graph/ge_local_context.h"
#include "register/optimization_option_registry.h"
#include "register/op_lib_register_impl.h"
#undef private
#undef protected
using namespace std;
using namespace fe;

class configuration_ut : public testing::Test {
 protected:
  static void SetUpTestCase() {
    fe::InitPlatformInfo("Ascend910B1", true);
    PlatformUtils::Instance().pm_item_vec_[static_cast<size_t>(PlatformUtils::PlatformInfoItem::IsaArchVersion)] =
        static_cast<int64_t>(ISAArchVersion::EN_ISA_ARCH_V300);
  }
  void SetUp() {
    map<string, string> options;
    options.emplace(ge::PRECISION_MODE, ALLOW_FP32_TO_FP16);
    options.emplace(ge::MODIFY_MIXLIST, GetCodeDir() + "/tests/engines/nn_engine/config/mix_list/op_mix_list1.json");
    ge::GetThreadLocalContext().SetGraphOption(options);
    Configuration &config = Configuration::Instance(fe::AI_CORE_NAME);
    config.is_init_ = false;
    string soc_version = "Ascend910B1";
    PlatformUtils::Instance().soc_version_ = soc_version;
    PlatformUtils::Instance().short_soc_version_ = "Ascend910B";
    config.Initialize(options);
    if (config.impl_mode_parser_ == nullptr) {
      config.impl_mode_parser_ = std::make_shared<OpImplModeConfigParser>(config.ascend_ops_path_);
    }
    if (config.cust_dtypes_parser_ == nullptr) {
      config.cust_dtypes_parser_ = std::make_shared<OpCustDtypesConfigParser>();
    }
    config.mix_list_parser_ = std::make_shared<ModifyMixlistConfigParser>();
    config.mix_list_parser_->InitializeFromOptions(options);
    if (config.op_debug_config_parse_ == nullptr) {
      config.op_debug_config_parse_ = std::make_shared<OpDebugConfigParser>();
    }
  }

  void TearDown() {
    map<string, string> options;
    options.emplace(ge::PRECISION_MODE, ALLOW_FP32_TO_FP16);
    ge::GetThreadLocalContext().SetGraphOption(options);
    Configuration &config = Configuration::Instance(fe::AI_CORE_NAME);
    config.is_init_ = false;
    string soc_version = "Ascend910B1";
    PlatformUtils::Instance().soc_version_ = soc_version;
    config.Initialize(options);
  }
  // AUTO GEN PLEASE DO NOT MODIFY IT
};

#endif  // TESTS_ENGINES_NN_ENGINE_UT_TESTCASE_FUSION_ENGINE_FUSION_MANAGER_UT_CONFIGURATION_FIXTURE_H_
