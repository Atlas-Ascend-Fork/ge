# -----------------------------------------------------------------------------------------------------------
# Copyright (c) 2025 Huawei Technologies Co., Ltd.
# This program is free software, you can redistribute it and/or modify it under the terms and conditions of
# CANN Open Software License Agreement Version 2.0 (the "License").
# Please refer to the License for details. You may not use this file except in compliance with the License.
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
# INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
# See LICENSE in the root of the software repository for the full text of the License.
# -----------------------------------------------------------------------------------------------------------

add_library(op_compile_adapter_headers INTERFACE)
target_include_directories(op_compile_adapter_headers INTERFACE
    $<BUILD_INTERFACE:${AIR_CODE_DIR}/compiler/opcompiler/inc>
    $<BUILD_INTERFACE:${AIR_CODE_DIR}/compiler/opcompiler/inc/tensor_engine>
    $<INSTALL_INTERFACE:include>
    $<INSTALL_INTERFACE:include/opcompiler>
    $<INSTALL_INTERFACE:include/opcompiler/tensor_engine>
)

file(GLOB OP_COMPILE_ADAPTER_SRC_CPP RELATIVE ${CMAKE_CURRENT_LIST_DIR}
     "common/*.cc"
     "cache/*.cc"
     "binary/*.cc"
     "compile/*.cc"
     "file_handle/*.cc"
     "dfxinfo_manager/*.cc"
     "assemble_json/*.cc"
     "python_adapter/*.cc"
     "api/*.cc"
    )

add_library(op_compile_adapter SHARED
    ${OP_COMPILE_ADAPTER_SRC_CPP}
    )

target_include_directories(op_compile_adapter PRIVATE
  ${HI_PYTHON_INC}
  ${CMAKE_CURRENT_LIST_DIR}
  ${AIR_CODE_DIR}/compiler/engines/nn_engine/utils/
  ${AIR_CODE_DIR}/compiler/engines/nn_engine/inc/
  )

target_compile_options(op_compile_adapter PRIVATE
  -fno-strict-aliasing
  )

target_compile_definitions(op_compile_adapter PRIVATE
      google=ascend_private
    )

target_link_libraries(op_compile_adapter
    PRIVATE
        ge_intf_pub
        ge_metadef_headers
        msprof_headers
        runtime_headers
        graph
        register
        static_mmpa
        unified_dlog
        c_sec
        json
        error_manager
        atrace_share
        aicore_utils
    PUBLIC
        op_compile_adapter_headers
)

if (BUILD_OPEN_PROJECT OR ENABLE_OPEN_SRC)
    install(TARGETS op_compile_adapter ${INSTALL_OPTIONAL}
            EXPORT op_compile_adapter-targets
            LIBRARY DESTINATION ${INSTALL_LIBRARY_DIR}
            )
endif()
