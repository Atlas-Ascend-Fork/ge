#!/bin/bash
set -e
# -----------------------------------------------------------------------------------------------------------
# Copyright (c) 2026 Huawei Technologies Co., Ltd.
# This program is free software, you can redistribute it and/or modify it under the terms and conditions of
# CANN Open Software License Agreement Version 2.0 (the "License").
# Please refer to the License for details. You may not use this file except in compliance with the License.
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
# INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
# See LICENSE in the root of the software repository for the full text of the License.
# -----------------------------------------------------------------------------------------------------------

if [ -z "${INSTALL_PATH:-}" ]; then
    echo "================ ge-compiler rpm/deb prerm skipped: INSTALL_PATH is empty ================"
    exit 0
fi

sourcedir="${INSTALL_PATH}"
# WHL_INSTALL_DIR_PATH=$(python3 -c "import sysconfig; print(sysconfig.get_path('purelib'))" 2>/dev/null)
WHL_INSTALL_DIR_PATH="${sourcedir}/python/site-packages"
export PYTHONPATH="${WHL_INSTALL_DIR_PATH}"
export PIP_BREAK_SYSTEM_PACKAGES=1

uninstall_local_package() {
    local package_name="$1"
    local distribution_name="${package_name//-/_}"
    # Restrict the package-manager operation to this CANN site-packages
    # root.  Falling back to the global pip database could remove a package
    # owned by another CANN installation or by the user.
    if [ ! -d "${WHL_INSTALL_DIR_PATH}/${package_name}" ] &&
        ! compgen -G "${WHL_INSTALL_DIR_PATH}/${distribution_name}-*.dist-info" >/dev/null 2>&1; then
        return 0
    fi
    pip3 uninstall -y "${package_name}" >/dev/null 2>&1 || true
}

for pkg in ge-py-pass-bridge ge-py-runtime-native ge-py-custom-op-bridge ge-py-onnx-plugin-bridge dataflow llm_datadist_v1 ge_py; do
    uninstall_local_package "${pkg}"
done

rm -fr "${WHL_INSTALL_DIR_PATH}/llm_datadist_v1" "${WHL_INSTALL_DIR_PATH}/dataflow" "${WHL_INSTALL_DIR_PATH}/ge_py" 2>/dev/null
rm -fr "${WHL_INSTALL_DIR_PATH}/ge/passes/python_pass_artifacts" 2>/dev/null
rm -fr "${WHL_INSTALL_DIR_PATH}/ge/runtime/python_runtime_artifacts" 2>/dev/null
rm -fr "${WHL_INSTALL_DIR_PATH}/ge/custom_op/python_custom_op_artifacts" 2>/dev/null
rm -fr "${WHL_INSTALL_DIR_PATH}/ge/onnx_plugin/python_onnx_plugin_artifacts" 2>/dev/null
rm -f "${WHL_INSTALL_DIR_PATH}/ge/passes/_ge_pass_native.so" 2>/dev/null
rm -fr "${WHL_INSTALL_DIR_PATH}/ge_py_pass_bridge-"*.dist-info 2>/dev/null
rm -fr "${WHL_INSTALL_DIR_PATH}/ge_py_runtime_native-"*.dist-info 2>/dev/null
rm -fr "${WHL_INSTALL_DIR_PATH}/ge_py_custom_op_bridge-"*.dist-info 2>/dev/null
rm -fr "${WHL_INSTALL_DIR_PATH}/ge_py_onnx_plugin_bridge-"*.dist-info 2>/dev/null

stub_libs="
libacl_op_compiler.so
libfmk_onnx_parser.so
libfmk_parser.so
libge_compiler.so
libge_runner.so
libge_runner_v2.so"

sourcedir="${INSTALL_PATH}"
pkg_arch_name="${PKG_ARCH_NAME}"

remove_stub_softlink() {
    local install_path="${sourcedir}"
    if [ ! -d "$install_path" ]; then
        return
    fi
    local devlibdir="${install_path}/${pkg_arch_name}-linux/devlib"
    ([ -d "${devlibdir}" ] && cd "${devlibdir}" && {
        chmod u+w . && echo "${stub_libs}" | xargs --no-run-if-empty rm -rf
        chmod u-w .
    }) || true
}

remove_stub_softlink

remove_empty_dir() {
    local dir="$1"
    local parent
    parent=$(dirname "${dir}")
    [ -d "${dir}" ] || return 0
    rmdir "${dir}" 2>/dev/null && chmod +w "${parent}" 2>/dev/null || true
}

remove_empty_dir "${WHL_INSTALL_DIR_PATH}"
remove_empty_dir "${sourcedir}/python"
