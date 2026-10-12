# -----------------------------------------------------------------------------------------------------------
# Copyright (c) 2025 Huawei Technologies Co., Ltd.
# This program is free software, you can redistribute it and/or modify it under the terms and conditions of
# CANN Open Software License Agreement Version 2.0 (the "License").
# Please refer to the License for details. You may not use this file except in compliance with the License.
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
# INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
# See LICENSE in the root of the software repository for the full text of the License.
# -----------------------------------------------------------------------------------------------------------

# ---- libom2_executor.so: 不再包含 dump/ 源码（profiling_* 已移至 profiling/ 目录，随 executor 编译），
# 运行时通过 dlopen 加载 libom2_dump.so ----
file(GLOB_RECURSE OM2_EXECUTOR_SRCS CONFIGURE_DEPENDS "${AIR_CODE_DIR}/runtime/om2/*.cc")
list(FILTER OM2_EXECUTOR_SRCS EXCLUDE REGEX "om2_model_manager\\.cc$")
list(FILTER OM2_EXECUTOR_SRCS EXCLUDE REGEX "/dump/.*\\.cc$")
set(OM2_EXTRA_SRCS
        "${AIR_CODE_DIR}/graph_metadef/base/utils/type_utils_inner.cc"
        "${AIR_CODE_DIR}/base/common/plugin/datatype_util.cc"
)
add_library(om2_executor SHARED
        ${OM2_EXECUTOR_SRCS}
        ${OM2_EXTRA_SRCS}
        # 此处仅添加执行时依赖的源文件
)

file(GLOB_RECURSE OM2_FORMAT_SRCS CONFIGURE_DEPENDS
        "${AIR_CODE_DIR}/runtime/om2/formats/*.cc"
        "${AIR_CODE_DIR}/runtime/om2/common/*.cc"
)
set_source_files_properties(${OM2_FORMAT_SRCS} PROPERTIES
        COMPILE_FLAGS "-fvisibility=hidden"
)

add_dependencies(om2_executor graphengine_protos)

target_compile_definitions(om2_executor PRIVATE
        google=ascend_private
        PROTOBUF_INLINE_NOT_IN_HEADERS=0
)

target_compile_definitions(om2_executor PRIVATE
        PROTOBUF_INLINE_NOT_IN_HEADERS=0
        google=ascend_private
        nlohmann=ascend_nlohmann
)

target_compile_options(om2_executor PRIVATE ${AIR_COMMON_COMPILE_OPTION}
        $<$<STREQUAL:${CMAKE_CXX_COMPILER_ID},Clang>:-fblocks>
        -fno-strict-aliasing
)

target_include_directories(om2_executor PRIVATE
        ${AIR_CODE_DIR}/runtime/om2/
        ${AIR_CODE_DIR}/runtime/v1/common/
        ${AIR_CODE_DIR}/base/
        ${AIR_CODE_DIR}/inc/
        ${AIR_CODE_DIR}/inc/framework
        ${AIR_CODE_DIR}/inc/external
        ${CMAKE_BINARY_DIR}
        ${CMAKE_BINARY_DIR}/proto/graphengine_protos
        ${METADEF_DIR}/inc/external
)

# libom2_executor.so 不再链接 ascend_protobuf / ascend_dump（含 adump_headers）：
# - dump 相关 protobuf（op_mapping/dump_task）与 adump API 已全部随 dump/ 源码归入 libom2_dump.so；
# - executor 仅经 inc/ 头文件传递性包含生成的 *.pb.h，故只需 protobuf 头文件路径
#   （ascend_protobuf_shared_headers，INTERFACE，不产生 libascend_protobuf.so 的 DT_NEEDED），
#   以及 graphengine_protos 的生成目录与构建顺序依赖；
# - profiling/ 仅调用 Msprof* 上报接口，由 profapi_share（libprofapi.so）提供，无需 libascend_dump.so。
target_link_libraries(om2_executor PRIVATE
        ${AIR_COMMON_LINK_OPTION}
        ge_intf_pub
        slog_headers
        msprof_headers
        ge_metadef_headers
        mmpa_headers
        runtime_headers
        error_manager
        c_sec
        runtime
        json
        minizip_static
        ascend_protobuf_shared_headers
        profapi_share
        acl_rt
        $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Android>>:-lrt>
        -ldl
)

# 需要调用 DataTypeUtil::GetIrDataType 把 GE 的枚举转为 proto 的枚举
set(OM2_DUMP_EXTRA_SRCS
        "${AIR_CODE_DIR}/base/common/plugin/datatype_util.cc"
)
# ---- libom2_dump.so: 独立编译 dump/ 源码，导出 C API 供 executor 以 dlopen 方式调用 ----
# 注意：profiling_* 源码已移入 profiling/ 目录并编入 libom2_executor.so，
# dump 库中对其符号的引用在运行时通过进程全局符号表解析（dlopen 时由 executor 提供）。
file(GLOB_RECURSE OM2_DUMP_SRCS CONFIGURE_DEPENDS "${AIR_CODE_DIR}/runtime/om2/dump/*.cc")
add_library(om2_dump SHARED
        ${OM2_DUMP_SRCS}
        ${OM2_DUMP_EXTRA_SRCS}
)

# 构建 om2_executor 时一并构建 om2_dump（仅构建依赖，非链接依赖），保证两个 .so 同目录产出。
add_dependencies(om2_executor om2_dump)

add_dependencies(om2_dump graphengine_protos)

target_compile_definitions(om2_dump PRIVATE
        PROTOBUF_INLINE_NOT_IN_HEADERS=0
        google=ascend_private
        nlohmann=ascend_nlohmann
)

target_compile_options(om2_dump PRIVATE ${AIR_COMMON_COMPILE_OPTION}
        $<$<STREQUAL:${CMAKE_CXX_COMPILER_ID},Clang>:-fblocks>
        -fno-strict-aliasing
)

target_include_directories(om2_dump PRIVATE
        ${AIR_CODE_DIR}/runtime/om2/
        ${AIR_CODE_DIR}/runtime/v1/common/
        ${AIR_CODE_DIR}/base/
        ${AIR_CODE_DIR}/inc/
        ${AIR_CODE_DIR}/inc/framework
        ${AIR_CODE_DIR}/inc/external
        ${CMAKE_BINARY_DIR}
        ${CMAKE_BINARY_DIR}/proto/graphengine_protos
        ${PROTOBUF_INCLUDE_DIR}
        ${METADEF_DIR}/inc/external
)

target_link_options(om2_executor PRIVATE
        # 治理静态链入的 gert_model_data_deserializer（C API/zip 类仅宿主内部消费）与 minizip/zlib 泄漏
        -Wl,--exclude-libs,libgert_model_data_deserializer.a
        -Wl,--exclude-libs,libminizip.a
        -Wl,--exclude-libs,libz.a
)

# libom2_dump.so 不反向依赖 libom2_executor.so：dump 侧仅导出句柄式 C API（Om2DumpManager*），
# executor 自行持有 ModelDumpManager 不透明句柄并在调用时传入，故此处无需 link om2_executor。
# 注意：构建产物 libom2_executor.so 与 libom2_dump.so 需部署到同一目录，
# executor 运行时通过 dladdr 定位自身路径并在同目录下 dlopen libom2_dump.so。
target_link_libraries(om2_dump PRIVATE
        ${AIR_COMMON_LINK_OPTION}
        ge_intf_pub
        slog_headers
        msprof_headers
        ge_metadef_headers
        adump_headers
        mmpa_headers
        gert_model_data_deserializer
        runtime_headers
        error_manager
        c_sec
        runtime
        json
        minizip_static
        ascend_protobuf
        ascend_dump
        acl_rt
        $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Android>>:-lrt>
        -ldl
)
