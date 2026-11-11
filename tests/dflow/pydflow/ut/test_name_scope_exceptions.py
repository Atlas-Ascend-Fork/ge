#!/usr/bin/env python3
# Copyright (c) 2026 Huawei Technologies Co., Ltd.
# This program is free software, you can redistribute it and/or modify it under the terms and conditions of
# CANN Open Software License Agreement Version 2.0 (the "License").
# Please refer to the License for details. You may not use this file except in compliance with the License.
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
# INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
# See LICENSE in the root of the software repository for the full text of the License.

"""Test name-scope recovery after duplicate aliases without the native extension."""

import importlib.util
from pathlib import Path
import sys
import unittest

SOURCE = next(
    (
        path
        for base in sys.path
        if (path := Path(base) / "dataflow/utils/name_scope.py").is_file()
    ),
    Path(__file__).resolve().parents[4]
    / "dflow/pydflow/python/dataflow/utils/name_scope.py",
)
SPEC = importlib.util.spec_from_file_location("dflow_scope_exception_test", SOURCE)
scope = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(scope)


class NameScopeExceptionTest(unittest.TestCase):
    def setUp(self):
        scope.reset_name_scope()

    def test_duplicate_node_alias_does_not_prefix_next_node(self):
        with scope.name_scope("shared", namespace="FlowNode", auto_name=False):
            pass
        with self.assertRaises(ValueError):
            with scope.name_scope("shared", namespace="FlowNode", auto_name=False):
                pass
        with scope.name_scope(None, "FlowNode", "FlowNode") as name:
            self.assertEqual(name, "FlowNode")

    def test_duplicate_nested_name_restores_outer_scope(self):
        with scope.name_scope("outer"):
            with scope.name_scope("shared", namespace="FlowNode", auto_name=False):
                pass
            with self.assertRaises(ValueError):
                with scope.name_scope("shared", namespace="FlowNode", auto_name=False):
                    pass
            with scope.name_scope("next") as name:
                self.assertEqual(name, "outer/next")
        with scope.name_scope("last") as name:
            self.assertEqual(name, "last")


if __name__ == "__main__":
    unittest.main()
