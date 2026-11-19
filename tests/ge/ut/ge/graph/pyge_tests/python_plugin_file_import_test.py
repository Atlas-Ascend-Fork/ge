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

import sys
import pytest
from ge._internal.plugin_loader import scan_modules_from_path


@pytest.mark.parametrize("fail", [False, True])
@pytest.mark.parametrize("parent_present", [False, True])
def test_file_plugin_imports_sibling_and_restores_path(tmp_path, fail, parent_present):
    (tmp_path / "_file_sibling_dependency.py").write_text(
        "VALUE = 7\n", encoding="utf-8"
    )
    entry = tmp_path / "file_plugin.py"
    code = "from _file_sibling_dependency import VALUE\n"
    if fail:
        code += "raise RuntimeError('plugin failed')\n"
    entry.write_text(code, encoding="utf-8")
    original_path = sys.path[:]
    if parent_present:
        sys.path.insert(0, str(tmp_path.resolve()))
    old_path = sys.path[:]
    try:
        if fail:
            with pytest.raises(RuntimeError, match="plugin failed"):
                scan_modules_from_path(
                    str(entry), module_prefix="_test_", plugin_kind="test plugin"
                )
        else:
            modules = scan_modules_from_path(
                str(entry), module_prefix="_test_", plugin_kind="test plugin"
            )
            assert modules[0].VALUE == 7
    finally:
        sys.modules.pop("_file_sibling_dependency", None)
        observed_path = sys.path[:]
        sys.path[:] = original_path
    assert observed_path == old_path
