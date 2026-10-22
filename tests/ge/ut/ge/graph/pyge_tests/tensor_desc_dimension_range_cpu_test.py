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
def desc_api(graph_api):
    _, native = graph_api
    native.GeApiWrapper_TensorDesc_Create.return_value = ctypes.c_void_p(1)
    native.GeApiWrapper_TensorDesc_SetShape.return_value = 0
    native.GeApiWrapper_TensorDesc_SetOriginShape.return_value = 0
    return load_source("ge.graph.tensor_desc", "graph/tensor_desc.py"), native


@pytest.mark.parametrize("dimension", [2**63, -(2**63) - 1, 2**64 + 7])
@pytest.mark.parametrize("entry", ["construct", "set_shape", "set_origin_shape"])
def test_out_of_range_dimensions_never_reach_native_boundary(
    desc_api, dimension, entry
):
    module, native = desc_api
    if entry == "construct":
        with pytest.raises(ValueError, match="signed int64"):
            module.TensorDesc([dimension])
        native.GeApiWrapper_TensorDesc_Create.assert_not_called()
    else:
        desc = module.TensorDesc([1])
        with pytest.raises(ValueError, match="signed int64"):
            getattr(desc, entry)([dimension])
        method = "SetShape" if entry == "set_shape" else "SetOriginShape"
        getattr(native, "GeApiWrapper_TensorDesc_" + method).assert_not_called()


def test_valid_boundaries_and_dynamic_dims_keep_exact_native_values(desc_api):
    module, native = desc_api
    dims = [-(2**63), 2**63 - 1, -2, -1, 0, 7]
    desc = module.TensorDesc(dims)
    assert list(native.GeApiWrapper_TensorDesc_Create.call_args.args[0]) == dims
    assert module.Shape(dims) == dims
    assert desc.set_shape([]) is desc


def test_shape_uses_same_dimension_validation(desc_api):
    module, _ = desc_api
    with pytest.raises(ValueError, match="signed int64"):
        module.Shape([2**64 + 7])
