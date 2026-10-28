#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# -------------------------------------------------------------------
# -----------------------------------------------------------------------------------------------------------
# Copyright (c) 2025 Huawei Technologies Co., Ltd.
# This program is free software, you can redistribute it and/or modify it under the terms and conditions of
# CANN Open Software License Agreement Version 2.0 (the "License").
# Please refer to the License for details. You may not use this file except in compliance with the License.
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
# INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
# See LICENSE in the root of the software repository for the full text of the License.
# -----------------------------------------------------------------------------------------------------------

import ctypes
import importlib.util
import sys
from pathlib import Path
from types import ModuleType
from unittest.mock import Mock

import pytest


@pytest.fixture
def graph_api(monkeypatch):
    # Load the real Python implementation with an isolated native API stub.
    root = Path(__file__).resolve().parents[6] / "api/python/ge/ge"
    for name in ("ge", "ge.graph", "ge._capi"):
        package = ModuleType(name)
        package.__path__ = []
        monkeypatch.setitem(sys.modules, name, package)
    wrapper = ModuleType("ge._capi.pygraph_wrapper")
    wrapper.graph_lib = Mock()
    monkeypatch.setitem(sys.modules, wrapper.__name__, wrapper)
    modules = {}
    for name in ("types", "_attr", "tensor_desc", "node"):
        full_name = "ge.graph." + name
        spec = importlib.util.spec_from_file_location(
            full_name, root / "graph" / (name + ".py")
        )
        module = importlib.util.module_from_spec(spec)
        monkeypatch.setitem(sys.modules, full_name, module)
        spec.loader.exec_module(module)
        modules[name] = module
    return modules, wrapper.graph_lib


@pytest.mark.parametrize(
    "type_name,getter,result,expected_type,expected_value",
    [
        ("VT_STRING", "get_string", "hello", "string", "'hello'"),
        ("VT_FLOAT", "get_float", 1.5, "float", "1.5"),
        ("VT_BOOL", "get_bool", False, "bool", "False"),
        ("VT_INT", "get_int", -7, "int", "-7"),
        ("VT_DATA_TYPE", "get_data_type", 3, "data_type", "3"),
        ("VT_TENSOR", "get_tensor", "Tensor(value=7)", None, "Tensor(value=7)"),
        ("VT_LIST_FLOAT", "get_list_float", [1.5, 2.5], "list_float", "[1.5, 2.5]"),
        ("VT_LIST_BOOL", "get_list_bool", [True, False], "list_bool", "[True, False]"),
        ("VT_LIST_INT", "get_list_int", [-1, 2], "list_int", "[-1, 2]"),
        ("VT_LIST_DATA_TYPE", "get_list_data_type", [0, 3], "list_data_type", "[0, 3]"),
        ("VT_LIST_STRING", "get_list_string", ["a", "b"], "list_string", "['a', 'b']"),
    ],
)
def test_attr_representation_dispatch(
    graph_api, monkeypatch, type_name, getter, result, expected_type, expected_value
):
    modules, native = graph_api
    value_type = getattr(modules["types"].AttrValueType, type_name)
    native.GeApiWrapper_AttrValue_GetValueType.return_value = value_type
    value = modules["_attr"]._AttrValue()
    read = Mock(return_value=result)
    monkeypatch.setattr(value, getter, read)
    if expected_type is None:
        expected_type = f"unknown({value_type})"
    expected = f"AttrValue(type: {expected_type}, value: {expected_value})"

    assert str(value) == expected
    read.assert_called_once_with()
    read.reset_mock()
    assert repr(value) == expected
    read.assert_called_once_with()


def test_attr_representation_unknown_type(graph_api):
    modules, native = graph_api
    value_type = modules["types"].AttrValueType.VT_NONE
    native.GeApiWrapper_AttrValue_GetValueType.return_value = value_type
    value = modules["_attr"]._AttrValue()
    expected = f"AttrValue(type: unknown({value_type}), value: <unknown>)"

    assert str(value) == expected
    assert repr(value) == expected


@pytest.mark.parametrize("failure", ["get_value_type", "get_int"])
def test_attr_representation_read_error(graph_api, monkeypatch, failure):
    modules, native = graph_api
    native.GeApiWrapper_AttrValue_GetValueType.return_value = modules[
        "types"
    ].AttrValueType.VT_INT
    value = modules["_attr"]._AttrValue()
    read = Mock(side_effect=RuntimeError("attribute read failed"))
    monkeypatch.setattr(value, failure, read)

    assert str(value) == "AttrValue(<error reading value>)"
    read.assert_called_once_with()
    read.reset_mock()
    assert repr(value) == "AttrValue(<error reading value>)"
    read.assert_called_once_with()


@pytest.mark.parametrize("handle", [None, ctypes.c_void_p()])
def test_node_rejects_null_handle(graph_api, handle):
    modules, native = graph_api
    native.reset_mock()

    with pytest.raises(ValueError, match="^Node handle cannot be None$"):
        modules["node"].Node._create_from(handle)

    assert native.mock_calls == []
