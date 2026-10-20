# -----------------------------------------------------------------------------------------------------------
# Copyright (c) 2025 Huawei Technologies Co., Ltd.
# This program is free software, you can redistribute it and/or modify it under the terms and conditions of
# CANN Open Software License Agreement Version 2.0 (the "License").
# Please refer to the License for details. You may not use this file except in compliance with the License.
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
# INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
# See LICENSE in the root of the software repository for the full text of the License.
# -----------------------------------------------------------------------------------------------------------

add_library(ops_deliverables_fixture SHARED ops_fixture.cc)
target_include_directories(ops_deliverables_fixture PRIVATE ${ASCEND_INSTALL_PATH}/x86_64-linux/include)
set_target_properties(ops_deliverables_fixture PROPERTIES PREFIX "" OUTPUT_NAME "ops_deliverables_fixture")

add_executable(st_ops_deliverables ops_deliverables_st.cc)
target_include_directories(st_ops_deliverables PRIVATE ${AIR_CODE_DIR}/inc ${AIR_CODE_DIR}/tests)
target_link_libraries(st_ops_deliverables PRIVATE ops_deliverables_test_support ge_metadef_headers intf_llt_pub
  opp_registry metadef GTestShared::gtest GTestShared::gtest_main dl)
add_dependencies(st_ops_deliverables ops_deliverables_fixture)
target_compile_definitions(st_ops_deliverables PRIVATE
  GE_OPS_DELIVERABLES_FIXTURE="$<TARGET_FILE:ops_deliverables_fixture>")
