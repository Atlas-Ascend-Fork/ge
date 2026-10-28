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
    runtime_wrapper = ModuleType("ge._capi.pytensor_runtime_wrapper")
    runtime_wrapper.tensor_runtime_lib = Mock()
    monkeypatch.setitem(sys.modules, runtime_wrapper.__name__, runtime_wrapper)

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


@pytest.mark.parametrize("build", [False, True])
def test_foreign_output_is_rejected_before_native_call(builder_api, build):
    api, native = builder_api
    source = api.GraphBuilder("source")
    receiver = api.GraphBuilder("receiver")
    tensor = source.create_input(0)
    with pytest.raises(ValueError, match="belong"):
        if build:
            receiver.build_and_reset(outputs=[tensor])
        else:
            receiver.set_graph_output(tensor, 0)
    native.EsSetGraphOutput.assert_not_called()
    native.EsBuildGraphAndReset.assert_not_called()


def test_same_builder_output_is_passed_to_actual_native_boundary(builder_api):
    api, native = builder_api
    builder = api.GraphBuilder("local")
    tensor = builder.create_input(0)
    builder.set_graph_output(tensor, 2)
    assert native.EsSetGraphOutput.call_args.args[0] is tensor._handle
    assert native.EsSetGraphOutput.call_args.args[1].value == 2


def test_same_builder_outputs_can_be_built(builder_api):
    api, native = builder_api
    builder = api.GraphBuilder("local")
    tensor = builder.create_input(0)
    graph = builder.build_and_reset(outputs=[tensor])
    assert graph._handle.value == 12288
    assert builder._is_built is True
    native.EsBuildGraphAndReset.assert_called_once_with(builder._handle)


class _AttributeTensor(ctypes.Structure):
    _fields_ = []


@pytest.mark.parametrize("scope", ["tensor", "node"])
@pytest.mark.parametrize(
    "kind,value,ctype",
    [
        ("int64", -(1 << 63), ctypes.c_int64),
        ("string", "\u5c5e\u6027\u503c", ctypes.c_char_p),
        ("bool", True, ctypes.c_bool),
    ],
)
@pytest.mark.parametrize("status", [0, 7])
def test_attribute_setters_preserve_typed_ctypes_arguments(
    builder_api, scope, kind, value, ctype, status
):
    api, native = builder_api
    storage = _AttributeTensor()
    pointer = ctypes.pointer(storage)
    native.EsCreateGraphInputWithDetails.return_value = pointer
    builder = api.GraphBuilder("attributes")
    tensor = builder.create_input(0)
    received = []
    signature = ctypes.CFUNCTYPE(
        ctypes.c_int, ctypes.POINTER(_AttributeTensor), ctypes.c_char_p, ctype
    )

    @signature
    def callback(raw_tensor, name, raw_value):
        received.append(
            (ctypes.cast(raw_tensor, ctypes.c_void_p).value, name, raw_value)
        )
        return status

    name = {"int64": "Int64", "string": "String", "bool": "Bool"}[kind]
    setattr(native, f"EsSet{name}AttrFor{scope.title()}", callback)
    setter = getattr(builder, f"set_{scope}_attr_{kind}")
    if status:
        # The native failure message reads the node name; isolate that unrelated lookup.
        native.EsGetProducer.return_value = 500
        graph_wrapper = sys.modules["ge._capi.pygraph_wrapper"]
        graph_wrapper.graph_lib.GeApiWrapper_GNode_GetName.return_value = b"input"
        with pytest.raises(RuntimeError, match=f"Failed to set {scope} attribute"):
            setter(tensor, "\u6743\u91cd", value)
    else:
        assert setter(tensor, "\u6743\u91cd", value) is None
    expected_value = value.encode("utf-8") if kind == "string" else value
    assert received == [
        (ctypes.addressof(storage), "\u6743\u91cd".encode("utf-8"), expected_value)
    ]
    assert tensor.get_owner_builder() is builder


def test_tensor_ctypes_parameter_is_borrowed_and_read_only(builder_api):
    api, native = builder_api
    storage = _AttributeTensor()
    pointer = ctypes.pointer(storage)
    native.EsCreateGraphInputWithDetails.return_value = pointer
    builder = api.GraphBuilder("parameter")
    tensor = builder.create_input(0)
    assert tensor._as_parameter_ is pointer
    assert ctypes.POINTER(_AttributeTensor).from_param(tensor) is pointer
    with pytest.raises(AttributeError):
        tensor._as_parameter_ = None
    assert tensor.get_owner_builder() is builder


@pytest.mark.parametrize("operation", ["add", "sub", "mul", "div"])
@pytest.mark.parametrize("scope", ["attributes", "controls", "both"])
@pytest.mark.parametrize("legacy_wrapper", [False, True])
def test_arithmetic_result_preserves_scopes_and_restores_context(
    builder_api, operation, scope, legacy_wrapper
):
    api, native = builder_api
    builder = api.GraphBuilder("scopes")
    left = builder.create_input(0)
    right = builder.create_input(1)
    generated = sys.modules[
        "ge._capi.pyes_graph_builder_wrapper"
    ].get_generated_lib.return_value
    operator = getattr(generated, "Es" + operation.title())
    operator.return_value = ctypes.c_void_p(200)
    native.EsGetProducer.return_value = 500
    native.EsAddControlEdge.return_value = 0
    graph_native = sys.modules["ge._capi.pygraph_wrapper"].graph_lib
    graph_native.GeApiWrapper_AttrValue_Create.return_value = 600
    graph_native.GeApiWrapper_AttrValue_SetInt.return_value = True
    # Exercise ctypes conversion rather than comparing the Python argument
    # captured by Mock: attribute wrappers may expose _as_parameter_.
    received_attributes = []
    signature = ctypes.CFUNCTYPE(
        ctypes.c_uint32, ctypes.c_void_p, ctypes.c_char_p, ctypes.c_void_p
    )

    @signature
    def set_attr(node, name, value_pointer):
        received_attributes.append((node, name, value_pointer))
        return 0

    graph_native.GeApiWrapper_GNode_SetAttr = set_attr
    has_attrs = scope in ("attributes", "both")
    has_controls = scope in ("controls", "both")

    def invoke():
        if legacy_wrapper:
            # Existing generated wrappers use the original builder method.
            handle = operator(left._handle, right._handle)
            result_holder = api.TensorHolder._create_from(handle, builder)
            return builder._apply_scope_infos_to_node(result_holder)
        return getattr(left, operation)(right)

    with api.attr_scope({"scope_tag": 7} if has_attrs else {}):
        with api.control_dependency_scope([right] if has_controls else []):
            result = invoke()

    operator.assert_called_once_with(left._handle, right._handle)
    assert result._as_parameter_.value == 200
    assert result.get_owner_builder() is builder
    if has_attrs:
        assert received_attributes == [(500, b"scope_tag", 600)]
        assert graph_native.GeApiWrapper_AttrValue_SetInt.call_args.args[1].value == 7
    else:
        assert received_attributes == []
    if has_controls:
        destination, sources, count = native.EsAddControlEdge.call_args.args
        assert destination is result._as_parameter_
        assert count == 1
        assert sources[0] == right._as_parameter_.value
        native.EsAddControlEdge.assert_called_once()
    else:
        native.EsAddControlEdge.assert_not_called()

    # A second operation outside the scopes must inherit neither attribute nor edge.
    invoke()
    assert len(received_attributes) == int(has_attrs)
    assert native.EsAddControlEdge.call_count == int(has_controls)
