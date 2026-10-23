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
import sys
from types import ModuleType
from unittest.mock import Mock
import importlib.util
from pathlib import Path

import pytest

GE_PATH = Path(__file__).resolve().parents[6] / "api/python/ge/ge"


def load_source(name, relative):
    spec = importlib.util.spec_from_file_location(name, GE_PATH / relative)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


@pytest.fixture
def tensor_api(monkeypatch):
    # Only the native boundary and descriptor classes are substituted.
    for name in ("ge", "ge.graph", "ge._capi"):
        package = ModuleType(name)
        package.__path__ = []
        monkeypatch.setitem(sys.modules, name, package)
    types = load_source("ge.graph.types", "graph/types.py")
    numeric = load_source("ge.graph._numeric", "graph/_numeric.py")
    monkeypatch.setitem(sys.modules, "ge.graph.types", types)
    monkeypatch.setitem(sys.modules, "ge.graph._numeric", numeric)
    descriptors = ModuleType("ge.graph.tensor_desc")
    descriptors.Shape = Mock()
    descriptors.TensorDesc = Mock()
    monkeypatch.setitem(sys.modules, descriptors.__name__, descriptors)
    for name, attribute in (
        ("pygraph_wrapper", "graph_lib"),
        ("pyes_graph_builder_wrapper", "esb_lib"),
        ("pytensor_runtime_wrapper", "tensor_runtime_lib"),
    ):
        wrapper = ModuleType("ge._capi." + name)
        setattr(wrapper, attribute, Mock())
        monkeypatch.setitem(sys.modules, wrapper.__name__, wrapper)
    tensor = load_source("ge.graph.tensor", "graph/tensor.py")
    return tensor, types


@pytest.fixture
def attr_api(tensor_api, monkeypatch):
    tensor, types = tensor_api
    monkeypatch.setitem(sys.modules, "ge.graph.tensor", tensor)
    attr = load_source("ge.graph._attr", "graph/_attr.py")
    monkeypatch.setitem(sys.modules, "ge.graph._attr", attr)
    native = attr.graph_lib
    for name in (
        "SetListDataType",
        "SetListInt",
        "SetListBool",
        "SetListFloat",
        "SetListString",
    ):
        getattr(native, "GeApiWrapper_AttrValue_" + name).return_value = 0
    native.GeApiWrapper_GNode_SetAttr.return_value = 0
    node = load_source("ge.graph.node", "graph/node.py")
    return attr, node, types, native


def test_node_attr_preserves_list_datatype_dispatch(attr_api):
    attr, node, types, native = attr_api
    values = [types.DataType.DT_FLOAT16, types.DataType.DT_INT32]
    owner = node.Node._create_from(ctypes.c_void_p(1))
    owner.set_attr("types", values)
    native.GeApiWrapper_AttrValue_SetListDataType.assert_called_once()
    native.GeApiWrapper_AttrValue_SetListInt.assert_not_called()
    buffer = native.GeApiWrapper_AttrValue_SetListDataType.call_args.args[1]
    assert buffer._type_ is ctypes.c_int
    assert list(buffer) == [value.value for value in values]
    native.GeApiWrapper_GNode_SetAttr.assert_called_once()


@pytest.mark.parametrize(
    "values,setter",
    [
        ([1, 2], "SetListInt"),
        ([True, False], "SetListBool"),
        ([1.0, 2.0], "SetListFloat"),
        (["float16", "int32"], "SetListString"),
    ],
)
def test_other_list_types_keep_their_native_dispatch(attr_api, values, setter):
    attr, _, _, native = attr_api
    value = attr._AttrValue()
    value.set_value(values)
    getattr(native, "GeApiWrapper_AttrValue_" + setter).assert_called_once()
    native.GeApiWrapper_AttrValue_SetListDataType.assert_not_called()
