#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# Copyright (c) 2026 Huawei Technologies Co., Ltd.
# This program is free software, you can redistribute it and/or modify it under the terms and conditions of
# CANN Open Software License Agreement Version 2.0 (the "License").
# Please refer to the License for details. You may not use this file except in compliance with the License.
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
# INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
# See LICENSE in the root of the software repository for the full text of the License.

"""Compile the production plugin loader and exercise it from each host Python.

Bridge registration, plugin dispatch and graph dependencies are stubbed;
the loader and stream-entry decision logic come from production sources.
This test does not require a toolkit, pybind11, pytest, or Python headers.
"""

import ctypes
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
from concurrent.futures import ThreadPoolExecutor
import unittest
from unittest import mock

ROOT = Path(__file__).resolve().parents[6]


@unittest.skipUnless(shutil.which("c++"), "needs a C++ compiler")
class LazyPythonPassLoadingTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory()
        root = Path(cls.temp.name)
        sources = {
            "ge/ge_api_types.h": """
#pragma once
#include <cstddef>
#include <cstdint>
namespace ge { using Status = uint32_t; constexpr Status SUCCESS = 0; }
""",
            "framework/common/debug/ge_log.h": """
#pragma once
extern int errors;
#define GELOGE(...) (++errors)
#define GELOGW(...) ((void)0)
#define GELOGI(...) ((void)0)
#define GELOGD(...) ((void)0)
#define GE_TRACE_START(...) ((void)0)
#define GE_COMPILE_TRACE_TIMESTAMP_END(...) ((void)0)
""",
            "common/checker.h": """
#pragma once
#include "framework/common/debug/ge_log.h"
#define GE_ASSERT_TRUE(expr) do { if (!(expr)) { ++errors; return 1; } } while (false)
#define GE_ASSERT_NOTNULL(ptr) GE_ASSERT_TRUE((ptr) != nullptr)
#define GE_ASSERT_SUCCESS(expr, ...) do { if ((expr) != 0) { return 1; } } while (false)
""",
            "register/custom_pass_helper.h": """
#pragma once
#include "ge/ge_api_types.h"
#include <memory>
#include <map>
#include <string>
extern int executions;
namespace ge {
struct TestGraph;
class StreamPassContext;
class CustomPassHelper {
 public:
  static CustomPassHelper &Instance() { static CustomPassHelper helper; return helper; }
  Status Load() { return SUCCESS; }
  Status Unload() { return SUCCESS; }
  Status Run(std::shared_ptr<TestGraph> &, StreamPassContext &, int, const std::map<std::string, bool> &,
             const std::map<std::string, int32_t> & = {}) {
    ++executions;
    return SUCCESS;
  }
};
}
""",
            "bridge_stub.cc": """
#include "pass_plugin_loader.h"
#include "python_pass_pybind_bridge.h"
int errors = 0;
int registrations = 0;
int resets = 0;
int shutdowns = 0;
int executions = 0;
using Callback = int (*)();
Callback callback = nullptr;
namespace ge { namespace fusion {
Status RegisterPythonPassesFromPlugin() { ++registrations; return callback(); }
void UnloadPythonPasses() { ++resets; }
void ShutdownPythonPassesForProcess() { ++shutdowns; }
}}
extern "C" {
void Configure(Callback fn) { callback = fn; errors = registrations = resets = shutdowns = executions = 0; }
int Load() { return ge::fusion::LoadPassPlugins(); }
int Ensure() { return ge::fusion::EnsurePythonPassesLoaded(); }
int Unload() { return ge::fusion::UnloadPassPlugins(); }
int Registrations() { return registrations; }
int Resets() { return resets; }
int Shutdowns() { return shutdowns; }
int Errors() { return errors; }
int Executions() { return executions; }
}
""",
        }
        # Compile the exact production mode resolver, parser and stream callback
        # entry with a minimal graph model; no graph algorithms are replaced here.
        stream_source = (
            ROOT / "compiler/graph/build/stream/stream_utils.cc"
        ).read_text()
        stream_header = (
            ROOT / "compiler/graph/build/stream/stream_utils.h"
        ).read_text()
        declaration_start = stream_header.index("enum class AutoMultistreamMode")
        declaration_end = stream_header.index("struct Subgraph {")
        declarations = stream_header[declaration_start:declaration_end]
        functions = "\n".join(
            re.search(pattern, stream_source, re.S | re.M).group(0)
            for pattern in (
                r"graphStatus StreamUtils::GetAutoMultistreamParallelMode\(const ComputeGraphPtr.*?^\}",
                r"graphStatus StreamUtils::ParseAutoMultistreamParallelMode\(.*?^\}",
                r"Status StreamUtils::RunCustomStreamPass\(.*?^\}",
            )
        )
        sources["stream_entry.cc"] = (
            """
#include <algorithm>
#include <cctype>
#include <vector>
#include "pass_plugin_loader.h"
#include "common/checker.h"
#include "framework/common/debug/ge_log.h"
#include "register/custom_pass_helper.h"
namespace ge {
using graphStatus = Status;
using char_t = char;
constexpr Status GRAPH_SUCCESS = 0;
constexpr Status GRAPH_FAILED = 1;
constexpr int kMaxAutoMultistreamNum = 64;
constexpr const char *kAutoMultistreamParallelModeOption = "ge.autoMultistreamParallelMode";
constexpr const char *AICORE_NUM = "ge.aicoreNum";
constexpr const char *kVectorCoreNum = "ge.vectorcoreNum";
struct TestGraph {
  std::map<std::string, std::string> attrs;
  std::shared_ptr<TestGraph> parent;
  std::string GetName() const { return "test"; }
  void DelAttr(const std::string &name) { attrs.erase(name); }
};
using ComputeGraphPtr = std::shared_ptr<TestGraph>;
struct AttrUtils {
  static bool GetStr(const ComputeGraphPtr &g, const std::string &key, std::string &value) {
    const auto found = g->attrs.find(key);
    if (found == g->attrs.end()) { return false; }
    value = found->second; return true;
  }
  static bool SetStr(const ComputeGraphPtr &g, const std::string &key, const std::string &value) {
    g->attrs[key] = value; return true;
  }
};
struct GraphUtils {
  static ComputeGraphPtr FindRootGraph(const ComputeGraphPtr &g) { return g->parent ? g->parent : g; }
};
struct GraphUtilsEx {
  static ComputeGraphPtr CreateGraphPtrFromComputeGraph(const ComputeGraphPtr &g) { return g; }
};
struct TestContext {
  std::string mode;
  Status GetOption(const std::string &, std::string &value) { value = mode; return GRAPH_SUCCESS; }
};
TestContext &GetContext() { static TestContext ctx; return ctx; }
struct CustomPassStage { static constexpr int kAfterAssignLogicStream = 1; };
class StreamPassContext {
 public:
  explicit StreamPassContext(int64_t id) : id_(id) {}
  int64_t GetCurrMaxStreamId() const { return id_; }
 private:
  int64_t id_;
};
namespace fusion {
struct FusionUtils { static std::map<std::string, bool> ParseFusionSwitch() { return {}; } };
struct FusionPriorityCache {
  static FusionPriorityCache &GetInstance() { static FusionPriorityCache instance; return instance; }
  std::map<std::string, int32_t> GetGraphFusionPriorityMap() const { return {}; }
};
}
Status ReportInvalidAutoMultistreamMode(const std::string &, const std::string &) { return GRAPH_FAILED; }
Status ConvertToInt32(const std::string &s, int32_t &out) {
  try { out = std::stoi(s); return SUCCESS; } catch (...) { return GRAPH_FAILED; }
}
"""
            + declarations
            + """
struct StreamUtils {
  static graphStatus GetAutoMultistreamParallelMode(const ComputeGraphPtr &, std::string &, bool &);
  static graphStatus ParseAutoMultistreamParallelMode(const std::string &, AutoMultistreamConfig &, bool);
  static Status RunCustomStreamPass(const ComputeGraphPtr &, int64_t &);
  static std::string TransVecToStr(const std::vector<int64_t> &) { return ""; }
};
"""
            + functions
            + """
}
extern "C" int RunGraph(const char *attribute, const char *option, bool subgraph) {
  auto root = std::make_shared<ge::TestGraph>();
  if (attribute != nullptr) { root->attrs[ge::kAutoMultistreamParallelModeOption] = attribute; }
  ge::GetContext().mode = option == nullptr ? "" : option;
  auto graph = root;
  if (subgraph) { graph = std::make_shared<ge::TestGraph>(); graph->parent = root; }
  int64_t next_id = 1;
  return ge::StreamUtils::RunCustomStreamPass(graph, next_id);
}
"""
        )
        for name, content in sources.items():
            path = root / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(content)
        source_dir = ROOT / "compiler/graph/fusion/pass"
        output = root / "loader_test.so"
        result = subprocess.run(
            [
                "c++",
                "-std=c++17",
                "-shared",
                "-Wl,-Bsymbolic",
                "-fPIC",
                "-pthread",
                "-Wall",
                "-Wextra",
                "-Werror",
                "-I",
                str(root),
                "-I",
                str(source_dir),
                str(source_dir / "pass_plugin_loader.cc"),
                str(root / "bridge_stub.cc"),
                str(root / "stream_entry.cc"),
                "-o",
                str(output),
            ],
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            universal_newlines=True,
        )
        if result.returncode:
            raise AssertionError(result.stdout)
        cls.library = ctypes.CDLL(str(output))
        cls.callback_type = ctypes.CFUNCTYPE(ctypes.c_int)
        cls.library.Configure.argtypes = [cls.callback_type]
        cls.library.RunGraph.argtypes = [
            ctypes.c_char_p,
            ctypes.c_char_p,
            ctypes.c_bool,
        ]

    @classmethod
    def tearDownClass(cls):
        cls.temp.cleanup()

    def setUp(self):
        self.environment = mock.patch.dict(os.environ)
        self.environment.start()
        os.environ.pop("ASCEND_GE_PY_PASS_PATH", None)
        self.references = 0
        self.status = 0
        self.callback = self.callback_type(lambda: self.status)
        self.library.Configure(self.callback)

    def tearDown(self):
        while self.references:
            self.unload()
        self.environment.stop()

    def load(self):
        status = self.library.Load()
        if status == 0:
            self.references += 1
        return status

    def unload(self):
        self.assertEqual(self.library.Unload(), 0)
        self.references -= 1

    def test_initialization_never_probes_unavailable_bridge(self):
        self.status = 1
        self.assertEqual(self.load(), 0)
        self.assertEqual(self.load(), 0)
        self.assertEqual(self.library.Registrations(), 0)
        self.assertEqual(self.library.Errors(), 0)
        self.unload()
        self.assertEqual(self.library.Shutdowns(), 0)
        self.unload()
        self.assertEqual(self.library.Resets(), 0)

    def test_explicit_user_pass_loads_during_initialization(self):
        os.environ["ASCEND_GE_PY_PASS_PATH"] = "user_pass.py"
        self.assertEqual(self.load(), 0)
        self.assertEqual(self.library.Registrations(), 1)
        self.assertEqual(self.library.Ensure(), 0)
        self.assertEqual(self.library.Registrations(), 1)

    def test_explicit_user_pass_failure_is_not_ignored(self):
        os.environ["ASCEND_GE_PY_PASS_PATH"] = "user_pass.py"
        self.status = 1
        self.assertNotEqual(self.load(), 0)
        self.assertEqual(self.library.Resets(), 1)
        self.status = 0
        self.assertEqual(self.load(), 0)
        self.assertEqual(self.library.Registrations(), 2)

    def test_concurrent_first_use_registers_once_and_final_unload_cleans(self):
        self.assertEqual(self.load(), 0)
        self.assertEqual(self.load(), 0)
        with ThreadPoolExecutor(max_workers=8) as pool:
            self.assertEqual(
                list(pool.map(lambda _: self.library.Ensure(), range(16))), [0] * 16
            )
        self.assertEqual(self.library.Registrations(), 1)
        self.unload()
        self.assertEqual(self.library.Resets(), 0)
        self.unload()
        self.assertEqual(self.library.Resets(), 1)
        self.assertEqual(self.library.Shutdowns(), 1)
        self.assertEqual(self.load(), 0)
        self.assertEqual(self.library.Registrations(), 1)
        self.assertEqual(self.library.Ensure(), 0)
        self.assertEqual(self.library.Registrations(), 2)

    def test_lazy_failure_is_reported_and_retry_can_succeed(self):
        self.assertEqual(self.load(), 0)
        self.status = 1
        self.assertNotEqual(self.library.Ensure(), 0)
        self.assertEqual(self.library.Resets(), 1)
        self.status = 0
        self.assertEqual(self.library.Ensure(), 0)
        self.assertEqual(self.library.Registrations(), 2)

    def test_loading_requires_an_active_plugin_lifetime(self):
        self.assertNotEqual(self.library.Ensure(), 0)
        self.assertEqual(self.library.Registrations(), 0)

    def test_host_python_dependency_check_is_deferred(self):
        def probe():
            try:
                import pybind11

                return 0 if Path(pybind11.get_include()).is_dir() else 1
            except ImportError:
                return 1

        self.callback = self.callback_type(probe)
        self.library.Configure(self.callback)
        self.assertEqual(self.load(), 0)
        self.assertEqual(self.library.Registrations(), 0)
        self.assertEqual(self.library.Errors(), 0)
        self.assertEqual(self.library.Ensure(), probe())
        self.assertEqual(self.library.Registrations(), 1)

    def test_non_dag_graph_modes_do_not_load_bridge(self):
        self.status = 1
        self.assertEqual(self.load(), 0)
        for attribute, option in (
            (None, None),
            (None, b"cv"),
            (b"", b"LoadBalance:2"),
            (b"default", b"LoadBalance:2"),
            (b"cv", b"MainStream:2"),
        ):
            self.assertEqual(self.library.RunGraph(attribute, option, False), 0)
        self.assertEqual(self.library.Registrations(), 0)
        self.assertEqual(self.library.Errors(), 0)
        self.assertEqual(self.library.Executions(), 5)

    def test_dag_graph_and_context_options_load_before_execution(self):
        self.assertEqual(self.load(), 0)
        for attribute, option, subgraph in (
            (None, b"LoadBalance:2", False),
            (b"MainStream:2", b"cv", False),
            (b"LoadBalance:2", None, True),
        ):
            self.assertEqual(self.library.RunGraph(attribute, option, subgraph), 0)
        self.assertEqual(self.library.Registrations(), 1)
        self.assertEqual(self.library.Executions(), 3)

    def test_dag_loading_failure_prevents_pass_execution(self):
        self.assertEqual(self.load(), 0)
        self.status = 1
        self.assertNotEqual(self.library.RunGraph(b"LoadBalance:2", None, False), 0)
        self.assertEqual(self.library.Registrations(), 1)
        self.assertEqual(self.library.Executions(), 0)

    def test_invalid_mode_fails_without_compiling_bridge(self):
        self.assertEqual(self.load(), 0)
        for mode in (b"LoadBalance", b"MainStream:0", b"LoadBalance:65", b"bad:2"):
            self.assertNotEqual(self.library.RunGraph(mode, None, False), 0)
        self.assertEqual(self.library.Registrations(), 0)
        self.assertEqual(self.library.Executions(), 0)


if __name__ == "__main__":
    unittest.main()
