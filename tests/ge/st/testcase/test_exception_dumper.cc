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
#include <unistd.h>
#include "macro_utils/dt_public_scope.h"
#include "ge/ge_api.h"
#include "graph/compute_graph.h"
#include "framework/common/framework_types_internal.h"
#include "graph/ge_local_context.h"
#include "common/dump/exception_dumper.h"
#include "macro_utils/dt_public_unscope.h"

#include "ge_graph_dsl/graph_dsl.h"
#include "depends/mmpa/src/mmpa_stub.h"
#include "framework/ge_runtime_stub/include/stub/gert_runtime_stub.h"
#include "common/debug/ge_log.h"
#include "runtime/subscriber/global_dumper.h"

namespace ge {
class STEST_exception_dumper : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() override {
    remove("valid_path");
  }
};

TEST_F(STEST_exception_dumper, dump_exception_constructor) {
  ExceptionDumper exception_dumper;
  EXPECT_EQ(exception_dumper.is_registered_, true);
  EXPECT_EQ(gert::GlobalDumper::GetInstance()->GetInnerExceptionDumpers().count(&exception_dumper), 1);
  ExceptionDumper exception_dumper1{false};
  EXPECT_EQ(exception_dumper1.is_registered_, false);
  EXPECT_EQ(gert::GlobalDumper::GetInstance()->GetInnerExceptionDumpers().count(&exception_dumper1), 0);
}

TEST_F(STEST_exception_dumper, log_exception_args_on_host) {
  OpDescInfo op_desc_info;
  op_desc_info.op_name = "test_op";
  op_desc_info.op_type = "test_type";
  op_desc_info.is_host_args = true;
  ExceptionDumper exception_dumper;
  exception_dumper.LogExceptionArgs(op_desc_info);
}

TEST_F(STEST_exception_dumper, dump_exception_workspace_with_addr) {
  ge::DumpProperties dump_properties;
  dump_properties.AddPropertyValue("ALL_MODEL_NEED_DUMP_AND_IT_IS_NOT_A_MODEL_NAME", {"test"});
  dump_properties.SetDumpMode("all");
  std::vector<uint8_t> workspace_stub(8U);
  OpDescInfo op_desc_info;
  op_desc_info.op_name = "Save";
  op_desc_info.op_type = "Save";
  op_desc_info.space_addrs = {workspace_stub.data()};
  op_desc_info.workspace_bytes = {8};
  ExceptionDumper exception_dumper;
  const std::string dump_path = "./st_dump_ws/";
  ASSERT_TRUE(ge::CreateDirectory(dump_path) == 0);
  const std::string dump_file = dump_path + "dumpfile";
  (void)exception_dumper.DumpExceptionWorkspace(op_desc_info, dump_file, false, dump_properties);
  (void)remove(dump_file.c_str());
  (void)rmdir(dump_path.c_str());
}
}  // namespace ge
