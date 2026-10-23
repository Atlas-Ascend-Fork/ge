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
def builder_api(monkeypatch):
    # Exercise actual Python classes. Only the native libraries are substituted.
    root = Path(__file__).resolve().parents[6] / "api/python/ge/ge"
    for name, relative in [
        ("ge", ""),
        ("ge.es", "es"),
        ("ge.graph", "graph"),
        ("ge._capi", "_capi"),
    ]:
        package = ModuleType(name)
        package.__path__ = [str(root / relative)]
        monkeypatch.setitem(sys.modules, name, package)
    native = Mock()
    native.EsCreateGraphBuilder.side_effect = [4096, 8192]
    native.EsCreateGraphInputWithDetails.return_value = ctypes.c_void_p(123)
    native.EsCreateScalarInt64.return_value = ctypes.c_void_p(124)
    native.EsSetGraphOutput.return_value = 0
    native.EsBuildGraphAndReset.return_value = 12288
    wrapper = ModuleType("ge._capi.pyes_graph_builder_wrapper")
    wrapper.esb_lib = native
    wrapper.EsCTensorHolderPtr = ctypes.c_void_p
    for name in ("c_bool", "c_float", "c_int32", "c_int64", "c_uint32", "c_uint64"):
        setattr(wrapper, name, getattr(ctypes, name))
    wrapper.DEFAULT_GENERATED_LIB_NAME = "libes_default.so"
    wrapper.MATH_LIB_NAME = "libes_math.so"
    wrapper.get_generated_lib = Mock()
    monkeypatch.setitem(sys.modules, wrapper.__name__, wrapper)
    graph_wrapper = ModuleType("ge._capi.pygraph_wrapper")
    graph_wrapper.graph_lib = Mock()
    monkeypatch.setitem(sys.modules, graph_wrapper.__name__, graph_wrapper)

    def load(name, relative):
        spec = importlib.util.spec_from_file_location(name, root / relative)
        module = importlib.util.module_from_spec(spec)
        monkeypatch.setitem(sys.modules, name, module)
        spec.loader.exec_module(module)
        return module

    load("ge.graph.types", "graph/types.py")
    load("ge.es.tensor_like", "es/tensor_like.py")
    holder = load("ge.es.tensor_holder", "es/tensor_holder.py")
    load("ge.graph._attr", "graph/_attr.py")
    load("ge.graph.node", "graph/node.py")
    graph = load("ge.graph.graph", "graph/graph.py")
    sys.modules["ge.graph"].Graph = graph.Graph
    sys.modules["ge.es"].TensorHolder = holder.TensorHolder
    api = load("ge.es.graph_builder", "es/graph_builder.py")
    return api, native


@pytest.mark.parametrize("bits", [32, 64])
@pytest.mark.parametrize("kind", ["negative", "overflow"])
@pytest.mark.parametrize("length", [1, 2])
def test_unsigned_constant_invalid_values_are_rejected_before_native(
    builder_api, bits, kind, length
):
    api, native = builder_api
    builder = api.GraphBuilder("range")
    invalid = -1 if kind == "negative" else 1 << bits
    values = [invalid] if length == 1 else [invalid, 0]
    with pytest.raises(ValueError, match=f"uint{bits}"):
        getattr(builder, f"create_const_uint{bits}")(values)
    getattr(native, f"EsCreateConstUInt{bits}").assert_not_called()
    getattr(native, f"EsCreateScalarUInt{bits}").assert_not_called()


@pytest.mark.parametrize("bits", [32, 64])
def test_valid_unsigned_list_boundaries_are_unchanged(builder_api, bits):
    api, native = builder_api
    builder = api.GraphBuilder("range")
    function = getattr(native, f"EsCreateConstUInt{bits}")
    function.return_value = ctypes.c_void_p(200)
    getattr(builder, f"create_const_uint{bits}")([0, (1 << bits) - 1])
    assert list(function.call_args.args[1]) == [0, (1 << bits) - 1]


def test_uint64_scalar_upper_bound_is_rejected(builder_api):
    api, native = builder_api
    builder = api.GraphBuilder("range")
    with pytest.raises(ValueError, match="uint64"):
        builder.create_scalar_uint64(1 << 64)
    native.EsCreateScalarUInt64.assert_not_called()


def test_ordinary_unsigned_scalar_still_uses_scalar_native_api(builder_api):
    api, native = builder_api
    builder = api.GraphBuilder("range")
    native.EsCreateScalarUInt64.return_value = ctypes.c_void_p(201)
    builder.create_const_uint64(42)
    assert native.EsCreateScalarUInt64.call_args.args[1].value == 42


@pytest.mark.parametrize(
    "kind,native_kind",
    [
        ("int64", "Int64"),
        ("int32", "Int32"),
        ("uint64", "UInt64"),
        ("uint32", "UInt32"),
    ],
)
@pytest.mark.parametrize(
    "value,shape,expected_values,expected_shape,scalar",
    [
        (7, None, [7], [], True),
        ([7], [], [7], [], True),
        ([3, 7], None, [3, 7], [2], False),
        ([3, 7], [1, 2], [3, 7], [1, 2], False),
        ([], [0], [], [0], False),
    ],
)
def test_integer_constant_normalization_preserves_native_buffers(
    builder_api,
    kind,
    native_kind,
    value,
    shape,
    expected_values,
    expected_shape,
    scalar,
):
    api, native = builder_api
    builder = api.GraphBuilder("constants")
    function = getattr(
        native, f"EsCreate{'Scalar' if scalar else 'Const'}{native_kind}"
    )
    function.return_value = ctypes.c_void_p(200)
    getattr(builder, f"create_const_{kind}")(value, shape)
    args = function.call_args.args
    if scalar:
        assert args[1].value == expected_values[0]
    else:
        assert list(args[1]) == expected_values
        assert list(args[2]) == expected_shape
        assert args[3].value == len(expected_shape)


@pytest.mark.parametrize("kind", ["int64", "int32", "uint64", "uint32"])
@pytest.mark.parametrize("value", [[1, "bad"], (1, 2), None])
def test_integer_constant_normalization_retains_type_errors(builder_api, kind, value):
    api, native = builder_api
    builder = api.GraphBuilder("types")
    with pytest.raises(TypeError, match="integer or list of integers"):
        getattr(builder, f"create_const_{kind}")(value)
    assert not any(call[0].startswith("EsCreateConst") for call in native.mock_calls)
