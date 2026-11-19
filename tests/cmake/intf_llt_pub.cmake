# -----------------------------------------------------------------------------------------------------------
# Copyright (c) 2025 Huawei Technologies Co., Ltd.
# This program is free software, you can redistribute it and/or modify it under the terms and conditions of
# CANN Open Software License Agreement Version 2.0 (the "License").
# Please refer to the License for details. You may not use this file except in compliance with the License.
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
# INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
# See LICENSE in the root of the software repository for the full text of the License.
# -----------------------------------------------------------------------------------------------------------
# 本模块会被多处 include()（tests/、tests/engines/、tests/acl_ut/ 等），而 include() 不产生新的作用域，
# 重复 add_library 同名 target 会触发 CMP0002 错误使 configure 失败。故整体加 NOT TARGET 保护，
# 让本模块幂等；调用方无论是否自带 if (NOT TARGET ...) 都可安全 include。
if (NOT TARGET intf_llt_pub)

add_library(intf_llt_pub INTERFACE)

target_include_directories(intf_llt_pub INTERFACE
        )

target_compile_definitions(intf_llt_pub INTERFACE
        CFG_BUILD_DEBUG
        _GLIBCXX_USE_CXX11_ABI=0
        FUNC_VISIBILITY
        FMK_SUPPORT_DUMP
        FWK_SUPPORT_TRAINING_TRACE
        )

target_compile_options(intf_llt_pub INTERFACE
        -g
        -O0
        -w
        $<$<COMPILE_LANGUAGE:CXX>:-std=c++17>
        $<$<BOOL:${ENABLE_ASAN}>:
        -Wno-maybe-uninitialized -fsanitize=address -fsanitize=leak -fsanitize-recover=address,all
            -fno-stack-protector -fno-omit-frame-pointer -g>
        -Werror -fno-common -Wextra -Wfloat-equal -Wall -fPIC
        -pipe
        -fno-access-control
        $<$<BOOL:${ENABLE_GCOV}>:--coverage -fprofile-arcs -ftest-coverage>
        )

target_link_options(intf_llt_pub INTERFACE
        $<$<BOOL:${ENABLE_ASAN}>:-fsanitize=address -fsanitize=leak -fsanitize-recover=address>
        $<$<BOOL:${ENABLE_GCOV}>:--coverage -fprofile-arcs -ftest-coverage>
        )

target_link_directories(intf_llt_pub INTERFACE
        )

target_link_libraries(intf_llt_pub INTERFACE
        -lpthread
        -ldl
        -lrt
        )

endif()
