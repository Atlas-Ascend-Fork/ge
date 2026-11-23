#!/bin/bash
# -----------------------------------------------------------------------------------------------------------
# Copyright (c) 2026 Huawei Technologies Co., Ltd.
# This program is free software, you can redistribute it and/or modify it under the terms and conditions of
# CANN Open Software License Agreement Version 2.0 (the "License").
# Please refer to the License for details. You may not use this file except in compliance with the License.
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
# INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
# See LICENSE in the root of the software repository for the full text of the License.
# -----------------------------------------------------------------------------------------------------------

set -euo pipefail

echo "start run test case, please wait ..."
cd ${WORKSPACE}

export ASCEND_GLOBAL_LOG_LEVEL=2
export ASCEND_SLOG_PRINT_TO_STDOUT=0
source /usr/local/Ascend/cann/set_env.sh

log() {
  local dt
  dt=$(date '+%Y%m%d.%H%M%S')
  echo "===================================================================="
  echo "$dt : $*"
  echo "===================================================================="
}

package_slog() {
  if [ ! -f "${WORKSPACE}/slog.tar.gz" ] && [ -d /root/ascend/log ]; then
    tar -zcf "${WORKSPACE}/slog.tar.gz" -C /root/ascend log || true
  fi
}
trap package_slog EXIT

log "init test case, please wait ..."
rm -rf /root/ascend/log

# ==============================
# 下载所需编译包
# ==============================
arm_package_dflow="cann-dflow_linux-aarch64.run"
arm_package_compiler="cann-ge-compiler_linux-aarch64.run"
arm_package_executor="cann-ge-executor_linux-aarch64.run"

# compile 阶段跳过时(仅改 md/docs 等文件, build.sh 返回 200), ge_compile.sh 会上传
# 0 字节的 empty.run 占位包。以 empty.run 为显式跳过标记: 三包全为占位则跳过冒烟;
# URL 为空/部分占位属 compile 产出链异常, 走下方下载失败分支 exit 1。
skip_count=0
for url in "${dflow_url:-}" "${compiler_url:-}" "${executor_url:-}"; do
  [ "$(basename "${url}")" = "empty.run" ] && skip_count=$((skip_count + 1))
done
if [ "${skip_count}" -eq 3 ]; then
  echo "[PreSmoke] Compile stage skipped (only md/docs changed, empty.run placeholder), no need to execute smoke."
  exit 0
fi

download_package() {
  local url="$1" package="$2"
  echo "Downloading ${package} from ${url}"
  if [ -z "${url}" ] || ! wget -nv --tries=3 --timeout=60 -O "${package}" "${url}"; then
    echo "[PreSmoke] ERROR: failed to download ${package} (url: ${url:-<empty>})" >&2
    return 1
  fi
  if [ ! -s "${package}" ]; then
    echo "[PreSmoke] ERROR: downloaded ${package} is empty (url: ${url})" >&2
    return 1
  fi
}

download_package "${dflow_url:-}" "${arm_package_dflow}" || exit 1
download_package "${compiler_url:-}" "${arm_package_compiler}" || exit 1
download_package "${executor_url:-}" "${arm_package_executor}" || exit 1

chmod +x "${arm_package_dflow}"
echo "y" | bash "${arm_package_dflow}" --full --install-path=/usr/local/Ascend --quiet

chmod +x "${arm_package_compiler}"
echo "y" | bash "${arm_package_compiler}" --full --install-path=/usr/local/Ascend --quiet

chmod +x "${arm_package_executor}"
echo "y" | bash "${arm_package_executor}" --full --install-path=/usr/local/Ascend --quiet




# ==============================
# 运行测试
# ==============================

# bash CI/cann/public/install_miniconda.sh
# source ~/.bashrc
log "start run test case, please wait ..."
source /usr/local/Ascend/cann/set_env.sh
cd examples/es/operator_overload_async/python && bash run_sample.sh -t sample_and_run_python 2>&1 | tee -a ${WORKSPACE}/run_test.log
cd - || exit 1

# ==============================
# 打包log
# ==============================
mkdir -p /root/ascend
cd ${WORKSPACE}
slog_name="slog.tar.gz"
tar -zcf "${slog_name}" -C /root/ascend log

# upload plog
# if python3 /home/upload.py --bucket-name "ascend-ci" --action upload  --local-file "slog.tar.gz" --obs-object-key "${repo_name}/package/${pr_id}/${slog_name}"; then
#   echo "::set-output var=plog_url:https://ascend-ci.obs.cn-north-4.myhuaweicloud.com/${repo_name}/package/${pr_id}/slog.tar.gz"
# fi

# ==============================
# 检查 NPU 状态
# ==============================
log "checking NPU status ..."
mkdir -p ./npu_log
npu-smi info  2>&1 | tee ./npu_log/npu_info.log

# ==============================
# 检查测试结果
# ==============================
log "checking test results ..."

date_time=$(date +%Y%m%d)"."$(date +%H%M%S)
if grep -iE '\b(FAIL|failed|error:)\b' "${WORKSPACE}/run_test.log" | grep -viE "error\)"; then
    echo "$date_time : run test case failed"
    exit 1
fi
