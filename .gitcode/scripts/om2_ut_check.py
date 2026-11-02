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
"""OM2 兼容性 UT 覆盖门禁检查脚本.

规则: inc/framework/om2/ 兼容性代码涉及的所有 struct 和字段(含枚举及枚举项)
必须在兼容性 UT(tests/ge/ut/ge/compatibility/om2/)中体现。符号提取与覆盖
判定复用公共模块 om2_symbol_coverage(clang AST dump + 词边界匹配, 不区分
断言形式与位置)。

始终阻断: UT 目录不存在/无文件、存在未覆盖符号、检查自身异常均判定失败
(exit 1), 仅全量符号覆盖时通过(exit 0)。
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
    read_dir_text,
)

# 兼容性 UT 目录, 位置如调整, 仅需修改此常量
UT_DIR = "tests/ge/ut/ge/compatibility/om2"
TARGET_DESC = "兼容性 UT"

logging.basicConfig(
    level=logging.INFO, format="[om2_ut_check] %(levelname)s: %(message)s"
)
logger = logging.getLogger("om2_ut_check")


def main():
    parser = argparse.ArgumentParser(description="OM2 compatibility UT coverage gate")
    parser.add_argument("--repo-root", default=".")
    parser.add_argument("--ut-dir", default=None, help="默认 %s" % UT_DIR)
    parser.add_argument("--clangxx", default=os.environ.get("OM2_CLANGXX", "clang++"))
    parser.add_argument(
        "--dump-symbols", action="store_true", help="仅导出符号清单到文件后退出"
    )
    parser.add_argument(
        "--dump-output", default="om2_symbols.json", help="符号清单导出文件路径"
    )
    args = parser.parse_args()

    ut_dir = args.ut_dir if args.ut_dir else os.path.join(args.repo_root, UT_DIR)

    def fail_result(reason):
        logger.error("%s", reason)
        return 1

    try:
        symbols = extract_symbols(args.repo_root, args.clangxx, DEFAULT_INCLUDES)
        if args.dump_symbols:
            dump_symbols_to_file(symbols, args.dump_output)
            return 0
        log_symbol_summary(symbols)
        ut_text = read_dir_text(ut_dir, "兼容性 UT")
        missing = check_coverage(symbols, ut_text, TARGET_DESC)
        if missing:
            logger.error("UT 未覆盖符号清单(%d 条):", len(missing))
            for item in missing:
                logger.error("  - %s", item)
            return fail_result(
                "inc/framework/om2 的 struct/字段必须在兼容性 UT(%s)中体现, "
                "请补充对应符号的布局/枚举值断言。" % UT_DIR
            )
        logger.info("PASS: 全量 struct/字段/枚举项均在兼容性 UT 中出现。")
        return 0
    except CheckError as exc:
        return fail_result(str(exc))
    except Exception as exc:  # noqa: BLE001 fail-close 兜底
        return fail_result("检查过程异常: %r" % exc)


if __name__ == "__main__":
    sys.exit(main())
