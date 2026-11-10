#!/usr/bin/env python3
# -*- coding: UTF-8 -*-
# -----------------------------------------------------------------------------------------------------------
# Copyright (c) 2026 Huawei Technologies Co., Ltd.
# This program is free software, you can redistribute it and/or modify it under the terms and conditions of
# CANN Open Software License Agreement Version 2.0 (the "License").
# Please refer to the License for details. You may not use this file except in compliance with the License.
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
# INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
# See LICENSE in the root of the software repository for the full text of the License.
# -----------------------------------------------------------------------------------------------------------

"""Regression checks for execution-graph methods in generated graph stubs."""

import pathlib
import re
import subprocess
import sys
import tempfile
import unittest


class GraphUtilsExStubTest(unittest.TestCase):
    def test_execution_graph_methods_have_returns(self):
        root = pathlib.Path(__file__).resolve().parents[3]
        generator = pathlib.Path(__file__).with_name("gen_stubapi.py")
        header = root / "inc/graph_metadef/graph/utils/graph_utils_ex.h"
        with tempfile.TemporaryDirectory() as output:
            subprocess.run(
                [sys.executable, str(generator), output, str(header)],
                check=True,
                capture_output=True,
                text=True,
            )
            stub = (pathlib.Path(output) / "stub_graph_utils_ex.cc").read_text()
        for method, expected in (
            ("Graph2ExecuteGraph", "return nullptr;"),
            ("ExecuteGraph2Graph", "return nullptr;"),
        ):
            with self.subTest(method=method):
                body = re.search(
                    rf"GraphUtilsEx::{method}\([^)]*\)\s*\{{(.*?)\n\}}",
                    stub,
                    re.DOTALL,
                )
                self.assertIsNotNone(body)
                self.assertEqual(body.group(1).strip(), expected)


if __name__ == "__main__":
    unittest.main()
