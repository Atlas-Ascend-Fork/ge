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
    echo "================ ge-compiler rpm/deb postinst skipped: INSTALL_PATH is empty ================"
    exit 0
fi

sourcedir="${INSTALL_PATH}"
pkg_arch_name="${PKG_ARCH_NAME}"
# WHL_INSTALL_DIR_PATH=$(python3 -c "import sysconfig; print(sysconfig.get_path('purelib'))" 2>/dev/null)
WHL_INSTALL_DIR_PATH="${sourcedir}/python/site-packages"
unset PYTHONPATH
export PIP_BREAK_SYSTEM_PACKAGES=1

WHL_LLM_DATADIST="${sourcedir}/${pkg_arch_name}-linux/lib64/llm_datadist_v1-0.0.1-py3-none-any.whl"
WHL_DATAFLOW="${sourcedir}/${pkg_arch_name}-linux/lib64/dataflow-0.0.1-py3-none-any.whl"
WHL_GE_PY="${sourcedir}/${pkg_arch_name}-linux/lib64/ge_py-0.0.1-py3-none-any.whl"
WHL_BRIDGE_DIR="${sourcedir}/${pkg_arch_name}-linux/lib64"

run_pip() { python3 -m pip "$@" || pip3 "$@"; }

install_required_wheel() {
    local wheel_path="$1"
    local package_name="$2"
    if [ ! -f "${wheel_path}" ]; then
        echo "[ge-compiler] ERROR: required ${package_name} wheel is missing: ${wheel_path}" >&2
        return 1
    fi
    if ! run_pip install --disable-pip-version-check --upgrade --no-deps --force-reinstall \
        -t "${WHL_INSTALL_DIR_PATH}" "${wheel_path}"; then
        echo "[ge-compiler] ERROR: failed to install required ${package_name} wheel: ${wheel_path}" >&2
        return 1
    fi
}

for whl in "${WHL_LLM_DATADIST}" "${WHL_DATAFLOW}"; do
    [ -f "${whl}" ] && echo "[ge-compiler] installing ${whl}" && run_pip install --disable-pip-version-check --upgrade --no-deps --force-reinstall -t "${WHL_INSTALL_DIR_PATH}" "${whl}" || true
done

if [ -f "${WHL_GE_PY}" ]; then
    py_tag=""
    py_abi_tag=""
    py_platform_tag=""
    if command -v python3 >/dev/null 2>&1 && python3 -m pip --version >/dev/null 2>&1; then
        py_tag=$(python3 -c 'import sys; print("cp%d%d" % sys.version_info[:2])' 2>/dev/null)
        # ABI tag 在 3.7 上是 cp37m，与 Python tag 不同；取不到时退回通配。
        py_abi_tag=$(python3 -c 'import sysconfig
parts = (sysconfig.get_config_var("SOABI") or "").split("-")
print("cp" + parts[1] if len(parts) > 1 and parts[0] == "cpython" else "")' 2>/dev/null)
        py_platform_tag=$(python3 -c 'import sysconfig
print(sysconfig.get_platform().replace("-", "_").replace(".", "_"))' 2>/dev/null)
    else
        py_tag=$(pip3 --version 2>/dev/null | sed -n 's/.*(python \([0-9][0-9]*\)\.\([0-9][0-9]*\)).*/cp\1\2/p' | head -n 1)
    fi
    bridge_whl=$(ls "${WHL_BRIDGE_DIR}"/ge_py_pass_bridge-*-"${py_tag}"-${py_abi_tag:-*}-${py_platform_tag:-*}.whl 2>/dev/null | head -1)
    runtime_whl=$(ls "${WHL_BRIDGE_DIR}"/ge_py_runtime_native-*-"${py_tag}"-${py_abi_tag:-*}-${py_platform_tag:-*}.whl 2>/dev/null | head -1)
    custom_op_whl=$(ls "${WHL_BRIDGE_DIR}"/ge_py_custom_op_bridge-*-"${py_tag}"-${py_abi_tag:-*}-${py_platform_tag:-*}.whl 2>/dev/null | head -1)
    onnx_plugin_whl=$(ls "${WHL_BRIDGE_DIR}"/ge_py_onnx_plugin_bridge-*-"${py_tag}"-${py_abi_tag:-*}-${py_platform_tag:-*}.whl 2>/dev/null | head -1)
    native_wheels=()
    [ -n "${bridge_whl}" ] && native_wheels+=("${bridge_whl}")
    [ -n "${runtime_whl}" ] && native_wheels+=("${runtime_whl}")
    [ -n "${custom_op_whl}" ] && native_wheels+=("${custom_op_whl}")
    [ -n "${onnx_plugin_whl}" ] && native_wheels+=("${onnx_plugin_whl}")
    if [ ${#native_wheels[@]} -ne 0 ]; then
        echo "[ge-compiler] installing ${WHL_GE_PY} with matching native wheels: ${native_wheels[*]}"
        if ! run_pip install --disable-pip-version-check --upgrade --no-index --no-deps --force-reinstall \
            -t "${WHL_INSTALL_DIR_PATH}" "${WHL_GE_PY}" "${native_wheels[@]}"; then
            echo "[ge-compiler] native wheel install failed, fallback to ge_py only"
            install_required_wheel "${WHL_GE_PY}" "ge_py"
        fi
    else
        echo "[ge-compiler] installing ${WHL_GE_PY}"
        install_required_wheel "${WHL_GE_PY}" "ge_py"
    fi
else
    echo "[ge-compiler] ERROR: required ge_py wheel is missing: ${WHL_GE_PY}" >&2
    exit 1
fi

stub_libs="
libacl_op_compiler.so
libfmk_onnx_parser.so
libfmk_parser.so
libge_compiler.so
libge_runner.so
libge_runner_v2.so"

sourcedir="${INSTALL_PATH}"
pkg_arch_name="${PKG_ARCH_NAME}"

create_stub_softlink() {
    local install_path="${sourcedir}"
    if [ ! -d "$install_path" ]; then
        return
    fi
    local devlibdir="${install_path}/${pkg_arch_name}-linux/devlib"
    ([ -d "${devlibdir}" ] && cd "${devlibdir}" && {
        chmod u+w . && \
        for lib in ${stub_libs}; do
            lib="linux/${pkg_arch_name}/$lib"
            [ -f "$lib" ] && ln -sf "$lib" "$(basename $lib)"
        done
        chmod u-w .
    }) || true
}

create_stub_softlink
