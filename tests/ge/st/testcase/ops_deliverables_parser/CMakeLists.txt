# -----------------------------------------------------------------------------------------------------------
# Copyright (c) 2026 Huawei Technologies Co., Ltd.
# This program is free software, you can redistribute it and/or modify it under the terms and conditions of
# CANN Open Software License Agreement Version 2.0 (the "License").
# Please refer to the License for details. You may not use this file except in compliance with the License.
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
# INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
# See LICENSE in the root of the software repository for the full text of the License.
# -----------------------------------------------------------------------------------------------------------

add_executable(st_ops_deliverables_parser ops_deliverables_parser_st.cc)
target_link_libraries(st_ops_deliverables_parser PRIVATE graph_base ge_metadef_headers intf_llt_pub
  GTestShared::gtest GTestShared::gtest_main)

include(CTest)
add_test(NAME st_ops_deliverables_parser
         COMMAND st_ops_deliverables_parser
                 --gtest_output=xml:${CMAKE_INSTALL_PREFIX}/report/st/ops_deliverables_parser.xml)
set_tests_properties(st_ops_deliverables_parser PROPERTIES
        LABELS "st;st_ge;st_ops_deliverables_parser"
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
        RUN_SERIAL TRUE
        TIMEOUT 7200)
