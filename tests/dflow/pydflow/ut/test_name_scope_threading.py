#!/usr/bin/env python3
# Copyright (c) 2026 Huawei Technologies Co., Ltd.
# This program is free software, you can redistribute it and/or modify it under the terms and conditions of
# CANN Open Software License Agreement Version 2.0 (the "License").
# Please refer to the License for details. You may not use this file except in compliance with the License.
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
# INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
# See LICENSE in the root of the software repository for the full text of the License.

"""Exercise DataFlow name scopes without requiring the native extension."""

from concurrent.futures import ThreadPoolExecutor
import importlib.util
from pathlib import Path
import sys
import threading
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
SPEC = importlib.util.spec_from_file_location("dflow_name_scope_test", SOURCE)
name_scope = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(name_scope)


def next_name():
    with name_scope.name_scope(None, "FlowData") as name:
        return name


class NameScopeThreadTest(unittest.TestCase):
    def setUp(self):
        name_scope.reset_name_scope()

    def test_workers_initialize_independent_scopes(self):
        barrier = threading.Barrier(2)

        def worker():
            barrier.wait(timeout=5)
            return next_name(), next_name()

        self.assertEqual(next_name(), "FlowData")
        with ThreadPoolExecutor(max_workers=2) as pool:
            futures = [pool.submit(worker) for _ in range(2)]
            self.assertEqual(
                [f.result(timeout=10) for f in futures],
                [("FlowData", "FlowData_1"), ("FlowData", "FlowData_1")],
            )
        self.assertEqual(next_name(), "FlowData_1")

    def test_worker_can_reinitialize_after_reset(self):
        with ThreadPoolExecutor(max_workers=1) as pool:
            self.assertEqual(pool.submit(next_name).result(timeout=10), "FlowData")
            name_scope.reset_name_scope()
            self.assertEqual(pool.submit(next_name).result(timeout=10), "FlowData")


if __name__ == "__main__":
    unittest.main()
