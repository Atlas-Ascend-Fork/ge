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


@pytest.mark.parametrize(
    "dtype_name,c_type,expected",
    [
        ("DT_FLOAT", ctypes.c_float, [1.0, 0.0]),
        ("DT_FLOAT16", ctypes.c_uint16, [0x3C00, 0]),
        ("DT_INT32", ctypes.c_int32, [1, 0]),
        ("DT_INT64", ctypes.c_int64, [1, 0]),
        ("DT_BOOL", ctypes.c_bool, [True, False]),
    ],
)
def test_boolean_values_use_declared_storage(tensor_api, dtype_name, c_type, expected):
    module, types = tensor_api
    dtype = getattr(types.DataType, dtype_name)
    tensor = module.Tensor([True, False], data_type=dtype, shape=[2])
    buffer = module.esb_lib.EsCreateEsCTensor.call_args.args[0]
    assert buffer._type_ is c_type
    assert ctypes.sizeof(buffer) == 2 * ctypes.sizeof(c_type)
    assert list(buffer) == expected
    assert module.esb_lib.EsCreateEsCTensor.call_args.args[3] == dtype
    del tensor


def test_bool_values_do_not_bypass_unsupported_dtype(tensor_api):
    module, types = tensor_api
    with pytest.raises(RuntimeError, match="DT_DOUBLE is not supported"):
        module.Tensor([True], data_type=types.DataType.DT_DOUBLE, shape=[1])
    module.esb_lib.EsCreateEsCTensor.assert_not_called()
