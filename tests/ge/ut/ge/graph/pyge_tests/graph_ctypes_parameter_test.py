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

"""Verify native pointer conversion and ownership at the Node C API boundary."""

import ctypes
import importlib
import sys
import types
from pathlib import Path
from unittest import mock

import pytest

import ge


@pytest.fixture
def graph_wrappers(monkeypatch):
    # Load wrappers under an isolated package without importing GE shared libraries.
    package = types.ModuleType("_ctypes_graph_test")
    package.__path__ = [str(Path(ge.__file__).parent / "graph")]
    monkeypatch.setitem(sys.modules, package.__name__, package)
    capi = types.ModuleType("ge._capi.pygraph_wrapper")
    capi.graph_lib = mock.Mock()
    capi.graph_lib.GeApiWrapper_AttrValue_Create.return_value = 0x1234
    capi.graph_lib.GeApiWrapper_TensorDesc_Create.return_value = 0x5678
    monkeypatch.setitem(sys.modules, capi.__name__, capi)
    try:
        module = importlib.import_module("_ctypes_graph_test.node")
        yield module, capi.graph_lib
    finally:
        for name in list(sys.modules):
            if name.startswith("_ctypes_graph_test."):
                del sys.modules[name]


@pytest.mark.parametrize(
    "operation", ["get_attr", "set_attr", "update_input_desc", "update_output_desc"]
)
def test_node_passes_borrowed_pointer_to_ctypes(graph_wrappers, monkeypatch, operation):
    module, native = graph_wrappers
    node = module.Node._create_from(0xABCD, owns_handle=False)
    seen = []
    is_attr = operation.endswith("attr")
    middle_type = ctypes.c_char_p if is_attr else ctypes.c_int32
    callback_type = ctypes.CFUNCTYPE(
        ctypes.c_int, ctypes.c_void_p, middle_type, ctypes.c_void_p
    )

    def capture(node_ptr, key_or_index, value_ptr):
        seen.append((node_ptr, key_or_index, value_ptr))
        return 0

    native_name = {
        "get_attr": "GeApiWrapper_GNode_GetAttr",
        "set_attr": "GeApiWrapper_GNode_SetAttr",
        "update_input_desc": "GeApiWrapper_GNode_UpdateInputDesc",
        "update_output_desc": "GeApiWrapper_GNode_UpdateOutputDesc",
    }[operation]
    setattr(native, native_name, callback_type(capture))
    if is_attr:
        monkeypatch.setattr(module._AttrValue, "get_value", lambda self: 42)
        monkeypatch.setattr(module._AttrValue, "set_value", lambda self, value: None)
        if operation == "get_attr":
            assert node.get_attr("key") == 42
        else:
            node.set_attr("key", 42)
        assert seen == [(0xABCD, b"key", 0x1234)]
        native.GeApiWrapper_AttrValue_Destroy.assert_called_once_with(0x1234)
    else:
        desc = module.TensorDesc()
        getattr(node, operation)(2, desc)
        assert seen == [(0xABCD, 2, 0x5678)]
        native.GeApiWrapper_TensorDesc_Destroy.assert_not_called()
        del desc
        native.GeApiWrapper_TensorDesc_Destroy.assert_called_once_with(0x5678)
    native.GeApiWrapper_GNode_DestroyGNode.assert_not_called()


def test_attribute_cache_keeps_successful_reads_and_invalidates_on_write(
    graph_wrappers,
):
    module, native = graph_wrappers
    current = {"value": 0}

    def read_int(_handle, result):
        ctypes.cast(result, ctypes.POINTER(ctypes.c_int64))[0] = current["value"]
        return 0

    native.GeApiWrapper_AttrValue_GetInt.side_effect = read_int
    native.GeApiWrapper_AttrValue_SetInt.return_value = 0
    native.GeApiWrapper_AttrValue_GetBool.return_value = 1
    attr = module._AttrValue()
    assert attr.get_int() == 0
    assert attr.get_int() == 0
    assert native.GeApiWrapper_AttrValue_GetInt.call_count == 1
    with pytest.raises(RuntimeError, match="Failed to get bool value"):
        attr.get_bool()
    assert attr.get_int() == 0
    assert native.GeApiWrapper_AttrValue_GetInt.call_count == 1
    current["value"] = 42
    assert attr.set_int(42) is True
    assert attr.get_int() == 42
    assert native.GeApiWrapper_AttrValue_GetInt.call_count == 2


def test_attribute_cache_does_not_reuse_a_different_type(graph_wrappers):
    module, native = graph_wrappers

    def read_bool(_handle, result):
        ctypes.cast(result, ctypes.POINTER(ctypes.c_bool))[0] = False
        return 0

    native.GeApiWrapper_AttrValue_GetBool.side_effect = read_bool
    native.GeApiWrapper_AttrValue_GetInt.return_value = 1
    attr = module._AttrValue()
    assert attr.get_bool() is False
    assert attr.get_bool() is False
    assert native.GeApiWrapper_AttrValue_GetBool.call_count == 1
    with pytest.raises(RuntimeError, match="Failed to get int value"):
        attr.get_int()
    native.GeApiWrapper_AttrValue_GetInt.assert_called_once()
