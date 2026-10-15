# -----------------------------------------------------------------------------------------------------------
# Copyright (c) 2025 Huawei Technologies Co., Ltd.
# This program is free software, you can redistribute it and/or modify it under the terms and conditions of
# CANN Open Software License Agreement Version 2.0 (the "License").
# Please refer to the License for details. You may not use this file except in compliance with the License.
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
# INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
# See LICENSE in the root of the software repository for the full text of the License.
# -----------------------------------------------------------------------------------------------------------

set(GE_METADEF_TEST_DIR ${AIR_CODE_DIR}/tests/graph_metadef)
set(AIR_COMMON_COMPILE_OPTION ${AIR_COMMON_COMPILE_OPTION} -fno-access-control)

include(cmake/intf_llt_pub.cmake)
add_cann_third_party(mockcpp)
add_subdirectory(depends/slog)
add_subdirectory(depends/mmpa)
add_subdirectory(depends/mmpa2)
add_subdirectory(depends/runtime)
add_subdirectory(depends/hccl)
add_subdirectory(depends/profiler)
add_subdirectory(depends/op_stub)
add_subdirectory(depends/graph_tuner)
add_subdirectory(depends/aoe)
add_subdirectory(depends/platform)
add_subdirectory(depends/llm_datadist)
add_subdirectory(depends/python)
add_subdirectory(depends/aihacb_autofusion)
add_subdirectory(depends/ascendcl)
option(OPS_DELIVERABLES_USE_INSTALLED_GRAPH "Test production OPP loader against installed graph dependencies" OFF)
add_library(ops_deliverables_test_support INTERFACE)
if (OPS_DELIVERABLES_USE_INSTALLED_GRAPH)
    # Component validation only: compile the unmodified production source, not a test replacement.
    add_library(ops_deliverables_test_impl STATIC
        ${GE_METADEF_DIR}/base/common/plugin/ops_deliverables_manager.cc)
    target_link_libraries(ops_deliverables_test_impl PUBLIC ge_metadef_headers intf_llt_pub
        "${ASCEND_INSTALL_PATH}/lib64/libgraph_base.so" metadef static_mmpa slog_stub unified_dlog opp_registry
        "${ASCEND_INSTALL_PATH}/lib64/libplatform.so" PRIVATE json runtime_headers)
    target_link_libraries(ops_deliverables_test_support INTERFACE ops_deliverables_test_impl)
else()
    target_link_libraries(ops_deliverables_test_support INTERFACE register)
endif()
add_subdirectory(framework)
add_subdirectory(benchmark)
add_subdirectory(ge)
add_subdirectory(bin)
add_subdirectory(parser)
add_subdirectory(graph_metadef)
add_subdirectory(dflow)
