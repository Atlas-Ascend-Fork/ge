#!/bin/bash
# ----------------------------------------------------------------------------
# Copyright (c) 2026 Huawei Technologies Co., Ltd.
# This program is free software, you can redistribute it and/or modify it under the terms and conditions of
# CANN Open Software License Agreement Version 2.0 (the "License").
# Please refer to the License for details. You may not use this file except in compliance with the License.
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
# INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
# See LICENSE in the root of the software repository for the full text of the License.
# ----------------------------------------------------------------------------
# ATC 转换脚本
#
# 用途：将 gen_onnx.py 生成的 ONNX 模型转换为 OM，并开启 DUMP_GE_GRAPH 导出图文件。
# 使用者可自行分析 dump 图中 Concat/Conv2D 节点上的 no-task 标记属性。
#
# 使用方式：
#   bash run_atc.sh <soc_version>
#   示例: bash run_atc.sh MC62
#
# 前置条件：
#   1. 已 source set_env.sh
#   2. 已安装 python3 + onnx
#   3. 已运行 python3 gen_onnx.py 生成 model.onnx
# ============================================================================

set -euo pipefail

SOC_VERSION="${1:?"Usage: $0 <soc_version>, e.g. $0 MC62"}"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
# dump目录使用绝对路径，保证清理/列举与dump落盘目录一致，不受执行时工作目录影响
DUMP_DIR="${SCRIPT_DIR}/ge_graph"

echo "============================================"
echo " ATC Convert + GE Graph Dump"
echo "============================================"
echo "SOC_VERSION: ${SOC_VERSION}"
echo "MODEL:       ${SCRIPT_DIR}/model.onnx"
echo "OUTPUT:      ${SCRIPT_DIR}/model"
echo ""

# 检查 ONNX 模型是否存在
if [ ! -f "${SCRIPT_DIR}/model.onnx" ]; then
    echo "[ERROR] model.onnx not found! Run 'python3 gen_onnx.py' first."
    exit 1
fi

# 清理上次产物
rm -f "${SCRIPT_DIR}/model.om"
rm -f "${DUMP_DIR}"/ge_onnx_*.pbtxt
rm -f "${DUMP_DIR}"/ge_proto_*.txt

# 设置 dump 环境变量
mkdir -p "${DUMP_DIR}"
export DUMP_GRAPH_PATH="${DUMP_DIR}"
export DUMP_GE_GRAPH=2
export DUMP_GRAPH_LEVEL=1
export ASCEND_SLOG_PRINT_TO_STDOUT=0
ATC_LOG_LEVEL="error"

echo "[1/2] Running ATC conversion..."
atc --model="${SCRIPT_DIR}/model.onnx" \
    --framework=5 \
    --soc_version="${SOC_VERSION}" \
    --output="${SCRIPT_DIR}/model" \
    --log=${ATC_LOG_LEVEL}

echo ""
echo "[2/2] ATC finished. Dump files:"
ls -lh "${DUMP_DIR}"/ge_proto_*.txt 2>/dev/null || echo "(no proto txt dump)"
ls -lh "${DUMP_DIR}"/ge_onnx_*.pbtxt 2>/dev/null || echo "(no onnx pbtxt dump)"
ls -lh "${SCRIPT_DIR}"/model.om 2>/dev/null || echo "(no om output)"

echo ""
echo "============================================"
echo " 分析提示："
echo "   grep '_no_task'                           ${DUMP_DIR}/ge_proto_*_RunCustomPass*.txt"
echo "   grep '_output_offset_list_for_continuous' ${DUMP_DIR}/ge_proto_*_RunCustomPass*.txt"
echo "============================================"
