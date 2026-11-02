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
"""OM2 兼容性符号提取与覆盖判定公共模块.

供 om2_doc_check.py(资料覆盖门禁)与 om2_ut_check.py(UT 覆盖门禁)复用:
  - 符号提取: 对 inc/framework/om2/ 头文件做 clang AST dump, 收集全部
    struct/union 名及字段名、枚举名及枚举项(仅 om2 头文件内的完整定义);
  - 覆盖判定: 每个符号名以词边界匹配在目标全文(资料/UT 文本)中查找,
    不区分位置; 存在未出现符号即判定未覆盖。
异常处理: fail-close, 统一抛 CheckError 由调用方判定失败。
"""

import json
import logging
import os
import re
import shutil
import subprocess
import tempfile

OM2_HEADERS = [
    "inc/framework/om2/model_api/om2_model_api.h",
    "inc/framework/om2/model_data/gert_model_data.h",
    "inc/framework/om2/model_data/om2_package_contants.h",
]
DEFAULT_INCLUDES = ["inc", "inc/framework", "inc/graph_metadef", "inc/external"]
MAX_STUB_RETRY = 30
RECORD_KINDS = ("RecordDecl", "CXXRecordDecl")

logger = logging.getLogger("om2_symbol_coverage")


class CheckError(Exception):
    """检查脚本自身异常, 按 fail-close 策略判定失败."""


def extract_symbols(repo_root, clangxx, includes):
    """对每个 om2 头文件执行 clang AST dump, 收集 struct/union/enum 符号.

    缺失的头文件告警跳过(兼容不同仓库/版本的 om2 目录子集), 全部缺失才视为异常。
    """
    header_paths = []
    for h in OM2_HEADERS:
        p = os.path.join(repo_root, h)
        if not os.path.isfile(p):
            logger.warning("头文件不存在, 跳过: %s", h)
            continue
        header_paths.append(os.path.realpath(p))
    if not header_paths:
        raise CheckError("OM2 头文件全部缺失, 无法提取符号")
    header_set = set(header_paths)
    collector = AstSymbolCollector(header_set)
    stub_dir = tempfile.mkdtemp(prefix="om2_symbol_coverage_stubs_")
    try:
        include_dirs = [os.path.join(repo_root, i) for i in includes]
        extra_env_dirs = [
            os.environ.get("ASCEND_INSTALL_PATH"),
            os.environ.get("ASCEND_3RD_LIB_PATH"),
        ]
        for base in extra_env_dirs:
            if base and os.path.isdir(os.path.join(base, "include")):
                include_dirs.append(os.path.join(base, "include"))
        include_dirs.append(stub_dir)
        for hp in header_paths:
            ast = run_clang_ast_dump(hp, include_dirs, stub_dir, clangxx)
            collector.reset_loc_state()
            collector.walk(ast, [])
    finally:
        shutil.rmtree(stub_dir, ignore_errors=True)
    symbols = collector.symbols
    if not symbols["structs"] and not symbols["enums"]:
        raise CheckError(
            "符号提取数为 0, clang 解析可能异常(检查镜像内 clang 可用性与头文件解析日志), "
            "禁止以空符号集判定覆盖通过"
        )
    return symbols


def run_clang_ast_dump(header, include_dirs, stub_dir, clangxx):
    if shutil.which(clangxx) is None and not os.path.isfile(clangxx):
        raise CheckError(
            "clang 不可用: %s 未找到, 请在具备 clang 的镜像中运行" % clangxx
        )
    # -ferror-limit=0: AST dump 只需解析完成即可收集符号名, 依赖头文件缺失导致的
    # 未知类型等错误不应累积触发 "too many errors" fatal 中止解析
    cmd = [
        clangxx,
        "-x",
        "c++",
        "-std=c++17",
        "-ferror-limit=0",
        "-Xclang",
        "-ast-dump=json",
    ]
    cmd += ["-I%s" % d for d in include_dirs]
    cmd.append(header)
    for _ in range(MAX_STUB_RETRY):
        proc = subprocess.run(cmd, capture_output=True, text=True, errors="replace")
        # fatal error 优先处理: clang 遇 fatal(如缺头文件)时仍可能输出中断前的部分 AST JSON,
        # 若先取 stdout 会拿到不完整符号集, 必须先走打桩重试
        m = re.search(r"fatal error: '([^']+)' file not found", proc.stderr)
        if m:
            create_stub(stub_dir, m.group(1))
            continue
        if re.search(r"fatal error:", proc.stderr):
            raise CheckError(
                "clang fatal error(%s): %s" % (header, proc.stderr[-2000:])
            )
        stdout = proc.stdout.strip()
        if stdout.startswith("{"):
            try:
                return json.loads(stdout)
            except json.JSONDecodeError as exc:
                raise CheckError("AST JSON 解析失败(%s): %s" % (header, exc)) from exc
        raise CheckError("clang AST dump 失败(%s): %s" % (header, proc.stderr[-2000:]))
    raise CheckError("缺失头文件打桩超过上限 %d 次(%s)" % (MAX_STUB_RETRY, header))


def create_stub(stub_dir, missing_header):
    stub_path = os.path.join(stub_dir, missing_header)
    os.makedirs(os.path.dirname(stub_path), exist_ok=True)
    if not os.path.exists(stub_path):
        with open(stub_path, "w", encoding="utf-8") as f:
            f.write("/* om2_symbol_coverage auto stub for %s */\n" % missing_header)


class AstSymbolCollector:
    """DFS 遍历 clang JSON AST, 收集 om2 头文件内定义的 struct/union/enum 符号.

    关键: clang JSON AST 的 loc 采用增量编码去重——file/line/col 等字段仅在
    与上一个输出的 loc 不同时才出现。因此须按 clang 输出流顺序(每个节点依次
    消费 loc、range.begin、range.end)维护 last_file 状态, 无 file 字段的 loc
    继承 last_file, 以此还原每个 decl 的真实所属文件。
    """

    def __init__(self, header_set):
        self.header_set = header_set
        self.last_file = None
        self.symbols = {"structs": {}, "enums": {}}

    def reset_loc_state(self):
        # 每个 header 是独立 clang 进程, LastLocFID 状态互不相通
        self.last_file = None

    def walk(self, node, ns_stack):
        if not isinstance(node, dict):
            return
        self._consume_loc(node)
        kind = node.get("kind")
        name = node.get("name")
        if kind == "NamespaceDecl" and name:
            for child in node.get("inner", []):
                self.walk(child, ns_stack + [name])
            return
        if kind in RECORD_KINDS and name and self._in_headers():
            # 真实 clang JSON AST 中前向声明不输出 completeDefinition 字段(None),
            # 完整定义输出 True; 仅收集完整定义, 过滤前向声明
            if node.get("completeDefinition") is not True:
                return
            qname = "::".join(ns_stack + [name])
            fields = []
            for child in node.get("inner", []):
                if (
                    isinstance(child, dict)
                    and child.get("kind") == "FieldDecl"
                    and child.get("name")
                ):
                    fields.append(child["name"])
            self.symbols["structs"].setdefault(qname, [])
            for f in fields:
                if f not in self.symbols["structs"][qname]:
                    self.symbols["structs"][qname].append(f)
        if kind == "EnumDecl" and name and self._in_headers():
            qname = "::".join(ns_stack + [name])
            constants = []
            for child in node.get("inner", []):
                if (
                    isinstance(child, dict)
                    and child.get("kind") == "EnumConstantDecl"
                    and child.get("name")
                ):
                    constants.append(child["name"])
            self.symbols["enums"].setdefault(qname, [])
            for c in constants:
                if c not in self.symbols["enums"][qname]:
                    self.symbols["enums"][qname].append(c)
        for child in node.get("inner", []):
            self.walk(child, ns_stack)

    def _consume_loc(self, node):
        for path in (("loc",), ("range", "begin"), ("range", "end")):
            cur = node
            for k in path:
                cur = cur.get(k) if isinstance(cur, dict) else None
                if cur is None:
                    break
            if isinstance(cur, dict) and "file" in cur:
                self.last_file = cur["file"]

    def _in_headers(self):
        return (
            bool(self.last_file) and os.path.realpath(self.last_file) in self.header_set
        )


def read_dir_text(dir_path, dir_desc):
    """读取目录下全部文件, 拼接为单一全文; 目录不存在或无文件视为异常."""
    if not os.path.isdir(dir_path):
        raise CheckError("%s目录不存在: %s" % (dir_desc, dir_path))
    texts = []
    for root, _, files in os.walk(dir_path):
        for fn in sorted(files):
            path = os.path.join(root, fn)
            with open(path, "r", encoding="utf-8", errors="replace") as f:
                texts.append(f.read())
    if not texts:
        raise CheckError("%s目录 %s 下未找到任何文件" % (dir_desc, dir_path))
    return "\n".join(texts)


def check_coverage(symbols, text, target_desc):
    """校验每个符号名(词边界)是否在目标全文中出现, 返回未覆盖清单."""
    missing = []
    for qname, fields in sorted(symbols["structs"].items()):
        bare = qname.split("::")[-1]
        if not re.search(r"\b%s\b" % re.escape(bare), text):
            missing.append("struct %s: 未在%s中出现" % (qname, target_desc))
            continue
        for field in fields:
            if not re.search(r"\b%s\b" % re.escape(field), text):
                missing.append("字段 %s.%s: 未在%s中出现" % (qname, field, target_desc))
    for qname, constants in sorted(symbols["enums"].items()):
        bare = qname.split("::")[-1]
        if not re.search(r"\b%s\b" % re.escape(bare), text):
            missing.append("enum %s: 未在%s中出现" % (qname, target_desc))
            continue
        for c in constants:
            if not re.search(r"\b%s\b" % re.escape(c), text):
                missing.append("枚举项 %s.%s: 未在%s中出现" % (qname, c, target_desc))
    return missing


def dump_symbols_to_file(symbols, dump_output):
    with open(dump_output, "w", encoding="utf-8") as f:
        json.dump(symbols, f, indent=2, ensure_ascii=False)
        f.write("\n")
    logger.info("符号清单已导出: %s", dump_output)


def log_symbol_summary(symbols):
    total = sum(len(v) for v in symbols["structs"].values()) + sum(
        len(v) for v in symbols["enums"].values()
    )
    logger.info(
        "符号提取完成: struct/union %d 个, enum %d 个, 字段/枚举项合计 %d 条。",
        len(symbols["structs"]),
        len(symbols["enums"]),
        total,
    )
