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
#include <nlohmann/json.hpp>
#include "fe_llt_utils.h"
#include "te_llt_utils.h"
#include "mmpa/mmpa_api.h"

using namespace std;

int main(int argc, char **argv) {
  string stub_cann_path = fe::GetCodeDir() + "/tests/engines/nn_engine/depends/CANN_910b_stub/cann";
  fe::EnvVarGuard cann_guard(MM_ENV_ASCEND_HOME_PATH, stub_cann_path.c_str());
  string stub_opp_path = fe::GetCodeDir() + "/tests/engines/nn_engine/depends/CANN_910b_stub/cann/opp";
  fe::EnvVarGuard opp_guard(MM_ENV_ASCEND_OPP_PATH, stub_opp_path.c_str());
  fe::InitPlatformInfo("Ascend910B1");
  testing::InitGoogleTest(&argc, argv);
  EXPECT_EQ(fe::InitPlatformInfo("Ascend910B1"), 0);
  // 不在此处提前 Restore：RUN_ALL_TESTS 期间必须保持 stub 环境。
  // 若提前恢复为 shell 的真实 ASCEND_OPP_PATH，真实 opp 包下存在 conf/ops_deliverables.json 时，
  // OpsDeliverableConfigManager 会劫持各测试 fixture 注册的假信息库（加载真实 ops-info json），
  // 导致规则解析 INVALID_RULE / ILLEGAL_RULE 断言失败、GetOpKernelInfoByOpType 返回空指针崩溃。
  // guard 析构（RUN_ALL_TESTS 之后）自动恢复原环境变量。
  int ret = RUN_ALL_TESTS();
  cann_guard.Restore();
  opp_guard.Restore();
  return ret;
}
