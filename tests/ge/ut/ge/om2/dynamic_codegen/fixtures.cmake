# Copyright (c) 2026 Huawei Technologies Co., Ltd.
# This program is free software, you can redistribute it and/or modify it under the terms and conditions of
# CANN Open Software License Agreement Version 2.0 (the "License").
# Please refer to the License for details. You may not use this file except in compliance with the License.
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
# INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
# See LICENSE in the root of the software repository for the full text of the License.

# Compile real generated models once per build, not once per test invocation.
set(om2_fixture_sources "${CMAKE_CURRENT_LIST_DIR}")
set(om2_fixture_dir "${CMAKE_CURRENT_BINARY_DIR}/om2_generated_fixtures")
add_executable(om2_generate_test_fixtures "${om2_fixture_sources}/generate_so_fixtures.cc")
get_target_property(om2_fixture_links ut_libge_common_utest LINK_LIBRARIES)
target_link_libraries(om2_generate_test_fixtures PRIVATE ${om2_fixture_links})
target_compile_definitions(om2_generate_test_fixtures PRIVATE
    google=ascend_private
    AIR_CODE_DIR="${AIR_CODE_DIR}"
    CMAKE_BINARY_DIR="${CMAKE_BINARY_DIR}"
)
target_compile_options(om2_generate_test_fixtures PRIVATE -g -O0 -Wno-deprecated-declarations)
target_include_directories(om2_generate_test_fixtures PRIVATE ${ASCEND_INSTALL_PATH}/include/exe_graph)
target_link_options(om2_generate_test_fixtures PRIVATE -rdynamic -Wl,-Bsymbolic -Wl,--disable-new-dtags)
add_dependencies(om2_generate_test_fixtures ge autofuse_stub)

set(om2_fixture_outputs "${om2_fixture_dir}/literals.cpp" "${om2_fixture_dir}/hccl.txt"
    "${om2_fixture_dir}/aicpu.txt")
foreach(model lifecycle_probe partial_init_failure registry_indirect registry_not_required
        branch_probe nested_branch_probe shared_library_probe)
    list(APPEND om2_fixture_outputs "${om2_fixture_dir}/${model}/${model}.so")
endforeach()
add_custom_command(
    OUTPUT ${om2_fixture_outputs}
    COMMAND ${CMAKE_COMMAND} -E env --unset=LD_LIBRARY_PATH --unset=ASCEND_OPP_PATH
        ASAN_OPTIONS=detect_leaks=0:halt_on_error=0 LSAN_OPTIONS=exitcode=0
        $<TARGET_FILE:om2_generate_test_fixtures> "${om2_fixture_dir}"
    DEPENDS om2_generate_test_fixtures om2_generator
    COMMENT "Generating and compiling OM2 test fixtures"
    VERBATIM
)
add_custom_command(
    OUTPUT "${om2_fixture_dir}/literals.so"
    COMMAND ${CMAKE_CXX_COMPILER} -std=c++17 -fPIC -shared
        "${om2_fixture_dir}/literals.cpp" -o "${om2_fixture_dir}/literals.so"
    DEPENDS "${om2_fixture_dir}/literals.cpp"
    VERBATIM
)
add_custom_target(om2_codegen_test_fixtures DEPENDS ${om2_fixture_outputs} "${om2_fixture_dir}/literals.so")

# Distinct SONAMEs isolate the success and missing-DT_NEEDED tests even under
# shuffle/repeat. Only $ORIGIN is searched; the build directory must not rescue
# the deliberately absent provider after the consumer is materialized in /tmp.
foreach(variant present missing)
    set(provider om2_fixture_provider_${variant})
    set(consumer om2_fixture_consumer_${variant})
    add_library(${provider} SHARED "${om2_fixture_sources}/shared_dependency_provider.cc")
    add_library(${consumer} SHARED "${om2_fixture_sources}/shared_dependency_consumer.cc")
    foreach(library ${provider} ${consumer})
        target_compile_definitions(${library} PRIVATE _GLIBCXX_USE_CXX11_ABI=0)
        target_include_directories(${library} BEFORE PRIVATE
            ${ASCEND_INSTALL_PATH}/include ${ASCEND_INSTALL_PATH}/pkg_inc ${ASCEND_INSTALL_PATH}/pkg_inc/base)
        target_link_directories(${library} PRIVATE ${ASCEND_INSTALL_PATH}/lib64)
        target_link_libraries(${library} PRIVATE -Wl,--whole-archive
            ${ASCEND_INSTALL_PATH}/lib64/librt2_registry.a -Wl,--no-whole-archive
            register metadef c_sec dl)
        set_target_properties(${library} PROPERTIES
            LIBRARY_OUTPUT_DIRECTORY "${om2_fixture_dir}/dependencies/${variant}"
            BUILD_WITH_INSTALL_RPATH TRUE INSTALL_RPATH "$ORIGIN" INSTALL_RPATH_USE_LINK_PATH FALSE)
    endforeach()
    set_target_properties(${provider} PROPERTIES OUTPUT_NAME om2_dependency_${variant})
    set_target_properties(${consumer} PROPERTIES OUTPUT_NAME om2_consumer)
    target_link_libraries(${consumer} PRIVATE -Wl,--no-as-needed ${provider})
    add_dependencies(om2_codegen_test_fixtures ${consumer})
endforeach()

target_compile_definitions(ut_libge_common_utest PRIVATE OM2_GENERATED_FIXTURE_DIR="${om2_fixture_dir}")
add_dependencies(ut_libge_common_utest om2_codegen_test_fixtures)
