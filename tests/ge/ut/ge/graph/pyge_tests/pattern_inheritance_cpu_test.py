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
def pattern_api(tensor_api, monkeypatch):
    from types import SimpleNamespace

    tensor, types = tensor_api
    monkeypatch.setitem(sys.modules, "ge.graph.tensor", tensor)
    attr = load_source("ge.graph._attr", "graph/_attr.py")
    monkeypatch.setitem(sys.modules, "ge.graph._attr", attr)
    node = load_source("ge.graph.node", "graph/node.py")
    monkeypatch.setitem(sys.modules, "ge.graph.node", node)
    graph = load_source("ge.graph.graph", "graph/graph.py")
    monkeypatch.setitem(sys.modules, "ge.graph.graph", graph)
    monkeypatch.setattr(sys.modules["ge.graph"], "Graph", graph.Graph, raising=False)
    monkeypatch.setattr(sys.modules["ge.graph"], "Node", node.Node, raising=False)
    for name in ("ge.es", "ge.passes"):
        package = ModuleType(name)
        package.__path__ = []
        monkeypatch.setitem(sys.modules, name, package)
    wrapper = sys.modules["ge._capi.pyes_graph_builder_wrapper"]
    for name in ("c_bool", "c_float", "c_int32", "c_int64", "c_uint32", "c_uint64"):
        monkeypatch.setattr(wrapper, name, getattr(ctypes, name), raising=False)
    monkeypatch.setattr(wrapper, "EsCTensorHolderPtr", ctypes.c_void_p, raising=False)
    monkeypatch.setattr(wrapper, "DEFAULT_GENERATED_LIB_NAME", "unused", raising=False)
    monkeypatch.setattr(wrapper, "MATH_LIB_NAME", "unused", raising=False)
    monkeypatch.setattr(wrapper, "get_generated_lib", Mock(), raising=False)
    for name in ("tensor_like", "tensor_holder", "graph_builder"):
        module = load_source("ge.es." + name, "es/" + name + ".py")
        monkeypatch.setitem(sys.modules, module.__name__, module)
    native = ModuleType("ge.passes._native")

    class NativePattern:
        def __init__(self, owner_graph):
            self.graph = owner_graph

    native.Pattern = NativePattern
    for name in (
        "MatchResult",
        "PassContext",
        "StreamPassContext",
        "PatternMatcherConfig",
        "PatternMatcherConfigBuilder",
        "SubgraphBoundary",
        "SubgraphInput",
        "SubgraphOutput",
        "SubgraphRewriter",
    ):
        setattr(native, name, type(name, (), {}))
    native.infer_shape = Mock()
    monkeypatch.setitem(sys.modules, native.__name__, native)
    spec = importlib.util.spec_from_file_location(
        "ge.passes.pattern", GE_PATH / "passes/pattern.py"
    )
    pattern = importlib.util.module_from_spec(spec)
    monkeypatch.setitem(sys.modules, pattern.__name__, pattern)
    spec.loader.exec_module(pattern)
    base = load_source("ge.passes.base", "passes/base.py")
    monkeypatch.setitem(sys.modules, base.__name__, base)
    return SimpleNamespace(
        pattern=pattern.pattern,
        Base=base.PatternFusionPass,
        Graph=graph.Graph,
        native=attr.graph_lib,
        NativePattern=NativePattern,
    )


def make_base(api):
    class Base(api.Base):
        @api.pattern
        def inherited(self, inputs):
            return api.Graph("base")

    return Base


def built_names(api, instance):
    api.native.GeApiWrapper_Graph_CreateGraph.reset_mock()
    result = instance.patterns()
    assert all(
        isinstance(item, api.NativePattern) and isinstance(item.graph, api.Graph)
        for item in result
    )
    return [
        call.args[0].decode("utf-8")
        for call in api.native.GeApiWrapper_Graph_CreateGraph.call_args_list
    ]


def test_child_pattern_extends_base_pattern_set(pattern_api):
    api = pattern_api
    Base = make_base(api)

    class Child(Base):
        @api.pattern
        def added(self, inputs):
            return api.Graph("child")

    assert built_names(api, Child()) == ["child", "base"]
    assert built_names(api, Base()) == ["base"]


def test_decorated_override_uses_only_child_implementation(pattern_api):
    api = pattern_api

    class Child(make_base(api)):
        @api.pattern
        def inherited(self, inputs):
            return api.Graph("overridden")

    assert built_names(api, Child()) == ["overridden"]


def test_shadowed_base_name_is_not_collected_when_child_adds_pattern(pattern_api):
    api = pattern_api

    class Child(make_base(api)):
        inherited = None

        @api.pattern
        def added(self, inputs):
            return api.Graph("child")

    assert built_names(api, Child()) == ["child"]


def test_diamond_inheritance_collects_base_pattern_once(pattern_api):
    api = pattern_api
    Base = make_base(api)

    class Left(Base):
        pass

    class Right(Base):
        pass

    class Child(Left, Right):
        @api.pattern
        def added(self, inputs):
            return api.Graph("child")

    assert built_names(api, Child()) == ["child", "base"]


def test_explicit_legacy_child_patterns_keeps_existing_style_selection(pattern_api):
    api = pattern_api

    class LegacyChild(make_base(api)):
        def patterns(self):
            return [api.Graph("legacy")]

    assert len(LegacyChild().patterns()) == 1
    assert api.native.GeApiWrapper_Graph_CreateGraph.call_args.args[0] == b"legacy"


def test_same_class_cannot_mix_pattern_declarations(pattern_api):
    api = pattern_api
    with pytest.raises(TypeError, match="cannot combine"):

        class Mixed(api.Base):
            @api.pattern
            def decorated(self, inputs):
                return api.Graph("unused")

            def patterns(self):
                return []
