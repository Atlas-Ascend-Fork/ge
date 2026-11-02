#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# -----------------------------------------------------------------------------------------------------------
# Copyright (c) 2026 Huawei Technologies Co., Ltd.
# This program is free software, you can redistribute it and/or modify it under the terms and conditions of
# CANN Open Software License Agreement Version 2.0 (the "License").
# Please refer to the License for details. You may not use this file except in compliance with the License.
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
# INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
# See LICENSE in the root of the software repository for the full text of the License.
# -----------------------------------------------------------------------------------------------------------
"""OM2 兼容性资料拦截检查脚本.

规则: inc/framework/om2/ 兼容性代码涉及的所有 struct 和字段(含枚举及枚举项)
必须在兼容性资料(归档于 docs 目录)中体现。符号提取与覆盖判定复用公共模块
om2_symbol_coverage(clang AST dump + 词边界匹配, 不区分章节等位置)。

始终阻断: 资料文件不存在、存在未覆盖符号、检查自身异常均判定失败(exit 1),
仅全量符号覆盖时通过(exit 0)。
"""

import argparse
import logging
import os
import sys

from om2_symbol_coverage import (
    DEFAULT_INCLUDES,
    CheckError,
    check_coverage,
    dump_symbols_to_file,
    extract_symbols,
    log_symbol_summary,
)

# 兼容性资料文件路径, 资料位置如调整, 仅需修改此常量
DOC_PATH = "docs/zh/design/features/om2_model_api.md"
TARGET_DESC = "兼容性资料"

logging.basicConfig(
    level=logging.INFO, format="[om2_doc_check] %(levelname)s: %(message)s"
)
logger = logging.getLogger("om2_doc_check")


def read_doc_text(doc_path):
    """读取兼容性资料文件全文."""
    with open(doc_path, "r", encoding="utf-8", errors="replace") as f:
        return f.read()


def main():
    parser = argparse.ArgumentParser(description="OM2 compatibility doc coverage gate")
    parser.add_argument("--repo-root", default=".")
    parser.add_argument("--doc-path", default=None, help="默认 %s" % DOC_PATH)
    parser.add_argument("--clangxx", default=os.environ.get("OM2_CLANGXX", "clang++"))
    parser.add_argument(
        "--dump-symbols", action="store_true", help="仅导出符号清单到文件后退出"
    )
    parser.add_argument(
        "--dump-output", default="om2_symbols.json", help="符号清单导出文件路径"
    )
    args = parser.parse_args()

    doc_path = (
        args.doc_path if args.doc_path else os.path.join(args.repo_root, DOC_PATH)
    )

    def fail_result(reason):
        logger.error("%s", reason)
        return 1

    try:
        symbols = extract_symbols(args.repo_root, args.clangxx, DEFAULT_INCLUDES)
        if args.dump_symbols:
            dump_symbols_to_file(symbols, args.dump_output)
            return 0
        log_symbol_summary(symbols)
        if not os.path.isfile(doc_path):
            return fail_result("兼容性资料文件不存在: %s" % doc_path)
        docs_text = read_doc_text(doc_path)
        missing = check_coverage(symbols, docs_text, TARGET_DESC)
        if missing:
            logger.error("资料未覆盖符号清单(%d 条):", len(missing))
            for item in missing:
                logger.error("  - %s", item)
            return fail_result(
                "inc/framework/om2 的 struct/字段必须在兼容性资料(%s)中体现, "
                "请在资料中补充对应符号的记载。" % DOC_PATH
            )
        logger.info("PASS: 全量 struct/字段/枚举项均在资料中出现。")
        return 0
    except CheckError as exc:
        return fail_result(str(exc))
    except Exception as exc:  # noqa: BLE001 fail-close 兜底
        return fail_result("检查过程异常: %r" % exc)


if __name__ == "__main__":
    sys.exit(main())
