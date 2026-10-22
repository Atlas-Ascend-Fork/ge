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
from types import ModuleType
from unittest.mock import Mock
from pathlib import Path

import pytest

GE_PATH = Path(__file__).resolve().parents[6] / "api/python/ge/ge"


def load_source(name, relative):
    spec = importlib.util.spec_from_file_location(name, GE_PATH / relative)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


@pytest.fixture
def graph_api(monkeypatch):
    for name in ("ge", "ge.graph", "ge._capi"):
        package = ModuleType(name)
        package.__path__ = []
        monkeypatch.setitem(sys.modules, name, package)
    types = load_source("ge.graph.types", "graph/types.py")
    monkeypatch.setitem(sys.modules, "ge.graph.types", types)
    wrapper = ModuleType("ge._capi.pygraph_wrapper")
    wrapper.graph_lib = Mock()
    monkeypatch.setitem(sys.modules, wrapper.__name__, wrapper)
    return types, wrapper.graph_lib


@pytest.fixture
def attr_api(graph_api):
    types, native = graph_api
    attr = load_source("ge.graph._attr", "graph/_attr.py")
    return attr, types, native


def test_datatype_list_roundtrips_as_enum_values(attr_api):
    attr, types, native = attr_api
    values = [types.DataType.DT_FLOAT16, types.DataType.DT_INT32]
    buffer = (ctypes.c_int * len(values))(*values)

    def read(_handle, size):
        ctypes.cast(size, ctypes.POINTER(ctypes.c_int64))[0] = len(values)
        return buffer

    native.GeApiWrapper_AttrValue_GetListDataType.side_effect = read
    native.GeApiWrapper_AttrValue_SetListDataType.return_value = 0
    value = attr._AttrValue()
    decoded = value.get_list_data_type()
    assert all(isinstance(item, types.DataType) for item in decoded)
    assert [item.name for item in decoded] == [item.name for item in values]
    assert value.set_list_data_type(decoded) is True
    native.GeApiWrapper_FreeListDataType.assert_called_once_with(buffer)


@pytest.mark.parametrize(
    ("getter", "free", "c_type", "values"),
    [
        ("GetListInt", "FreeListInt", ctypes.c_int64, [1, 2]),
        ("GetListFloat", "FreeListFloat", ctypes.c_float, [1.5, 2.5]),
        ("GetListBool", "FreeListBool", ctypes.c_bool, [True, False]),
    ],
)
def test_other_list_types_keep_native_values(attr_api, getter, free, c_type, values):
    attr, _, native = attr_api
    buffer = (c_type * len(values))(*values)

    def read(_handle, size):
        ctypes.cast(size, ctypes.POINTER(ctypes.c_int64))[0] = len(values)
        return buffer

    getattr(native, "GeApiWrapper_AttrValue_" + getter).side_effect = read
    value = attr._AttrValue()
    method = {
        "GetListInt": "get_list_int",
        "GetListFloat": "get_list_float",
        "GetListBool": "get_list_bool",
    }[getter]
    assert getattr(value, method)() == values
    getattr(native, "GeApiWrapper_" + free).assert_called_once_with(buffer)


def test_invalid_native_enum_still_frees_array(attr_api):
    attr, _, native = attr_api
    buffer = (ctypes.c_int * 1)(2147483647)

    def read(_handle, size):
        ctypes.cast(size, ctypes.POINTER(ctypes.c_int64))[0] = 1
        return buffer

    native.GeApiWrapper_AttrValue_GetListDataType.side_effect = read
    with pytest.raises(ValueError):
        attr._AttrValue().get_list_data_type()
    native.GeApiWrapper_FreeListDataType.assert_called_once_with(buffer)
