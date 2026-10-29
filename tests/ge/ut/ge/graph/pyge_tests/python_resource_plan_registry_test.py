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

"""Registration-time gates for ``register_resource_plan_pass``.

Every one of these must fail while the decorator runs, never at graph compile
time: a descriptor that cannot execute must not reach ``CustomPassHelper`` in
the first place.  The four negative cases below are the ones the design calls
out -- unknown kind, class not derived from the kind's base, a stage the kind
does not allow, and a duplicate pass name.
"""

import importlib

import pytest


try:
    base = importlib.import_module("ge.passes.base")
    registry = importlib.import_module("ge.passes.registry")
except ImportError as exc:
    pytest.skip(f"无法导入 GE pass registry: {exc}", allow_module_level=True)

PassStage = base.PassStage
ResourcePlanPass = base.ResourcePlanPass
StreamPass = base.StreamPass
FusionBasePass = base.FusionBasePass
DecomposePass = base.DecomposePass


@pytest.fixture(autouse=True)
def clean_registry():
    """The registry is process-global; keep each case independent."""

    registry.clear_registered_passes()
    yield
    registry.clear_registered_passes()


def _register(name, stage=PassStage.AFTER_ASSIGN_LOGIC_STREAM, kind="stream"):
    return registry.register_resource_plan_pass(
        name=name, pass_stage=stage, descriptor_kind=kind
    )


# ── 正面：装饰器产生可执行的 descriptor ────────────────────────────────


def test_registers_a_stream_descriptor_with_all_three_fields():
    @_register("GateStreamPass")
    class GatePass(StreamPass):
        def run(self, graph, context):
            return 0

    descriptor = GatePass.__ge_pass_descriptor__
    assert descriptor.pass_name == "GateStreamPass"
    assert descriptor.kind == "stream"
    assert descriptor.stage == PassStage.AFTER_ASSIGN_LOGIC_STREAM
    assert descriptor.cls is GatePass
    # The bridge hands this dict to native; kind is what routes the handler.
    assert descriptor.to_bridge_dict()["kind"] == "stream"
    assert (
        registry.get_registered_pass_by_descriptor_key(descriptor.descriptor_key)
        is descriptor
    )


def test_builtin_minidag_pass_name_is_preserved():
    """metadef skips by pass name, so this string is part of the contract."""

    stream_pass = pytest.importorskip("ge.passes._minidag_stream_pass")
    # Importing the module registered the built-in descriptor into the
    # registry the fixture just cleared, so re-run the decorator explicitly.
    descriptor = stream_pass.DAGStreamPass.__ge_pass_descriptor__
    assert descriptor.pass_name == "MiniDAGStreamPass"
    assert descriptor.kind == "stream"
    assert descriptor.stage == PassStage.AFTER_ASSIGN_LOGIC_STREAM


# ── 反面 1：未知 kind ───────────────────────────────────────────────────


@pytest.mark.parametrize("kind", ["memory", "event", "Stream", ""])
def test_unknown_descriptor_kind_is_rejected_at_registration(kind):
    with pytest.raises((ValueError, TypeError)) as exc:
        _register("UnknownKindPass", kind=kind)
    # The message must list the legal kinds; "register it now and fail later
    # when no handler is found" is explicitly not allowed.
    if isinstance(exc.value, ValueError):
        assert "stream" in str(exc.value)


# ── 反面 2：类与该 kind 的基类不匹配 ───────────────────────────────────


def test_non_resource_plan_class_is_rejected():
    with pytest.raises(TypeError, match="StreamPass"):

        @_register("NotAPassAtAll")
        class NotAPass:
            def run(self, graph, context):
                return 0


def test_resource_plan_base_alone_is_not_enough_for_stream_kind():
    """Checking ResourcePlanPass would let a future MemoryPlanPass through.

    It has to be the kind's own base class, or the pass reaches ``run`` with a
    context type it cannot use.
    """

    with pytest.raises(TypeError, match="StreamPass"):

        @_register("BareResourcePlanPass")
        class BarePass(ResourcePlanPass):
            def run(self, graph, context):
                return 0


def test_fusion_pass_cannot_be_registered_as_a_stream_pass():
    with pytest.raises(TypeError, match="StreamPass"):

        @_register("FusionAsStreamPass")
        class Wrong(FusionBasePass):
            def run(self, graph, context):
                return 0


# ── 反面 3：该 kind 不允许的阶段 ───────────────────────────────────────


@pytest.mark.parametrize(
    "stage",
    [
        PassStage.BEFORE_INFER_SHAPE,
        PassStage.AFTER_INFER_SHAPE,
        PassStage.AFTER_BUILTIN_FUSION_PASS,
        PassStage.AFTER_ORIGIN_GRAPH_OPTIMIZE,
    ],
)
def test_stage_outside_the_kind_allow_list_is_rejected(stage):
    with pytest.raises(ValueError, match="does not allow pass stage"):

        @_register("WrongStagePass", stage=stage)
        class WrongStage(StreamPass):
            def run(self, graph, context):
                return 0


# ── 反面 4：重名 ───────────────────────────────────────────────────────


def test_duplicate_pass_name_is_rejected_even_across_classes():
    """Pass names are globally unique, not unique per (kind, name, stage).

    Two downstream consumers identify a pass by name alone: metadef's
    registry and ``ShouldSkipMiniDagStreamPass``.
    """

    @_register("DuplicateNamePass")
    class First(StreamPass):
        def run(self, graph, context):
            return 0

    with pytest.raises(ValueError, match="already exists"):

        @_register("DuplicateNamePass")
        class Second(StreamPass):
            def run(self, graph, context):
                return 0

    # The failed registration must not leave a half-built descriptor behind.
    names = [d.pass_name for d in registry.get_registered_passes()]
    assert names.count("DuplicateNamePass") == 1


# ── 普通 pass 注册器必须拒绝流阶段（§3.2 第 3 条）─────────────────────


def test_register_fusion_pass_rejects_the_stream_stage():
    with pytest.raises(ValueError, match="register_resource_plan_pass"):
        registry.register_fusion_pass(
            name="FusionAtStreamStage", stage=PassStage.AFTER_ASSIGN_LOGIC_STREAM
        )


def test_register_decompose_pass_rejects_the_stream_stage():
    with pytest.raises(ValueError, match="AFTER_ASSIGN_LOGIC_STREAM"):
        registry.register_decompose_pass(
            name="DecomposeAtStreamStage",
            stage=PassStage.AFTER_ASSIGN_LOGIC_STREAM,
            op_types=["Add"],
        )


def test_register_fusion_pass_rejects_the_stream_kind():
    with pytest.raises(ValueError, match="register_resource_plan_pass"):
        registry.register_fusion_pass(
            name="FusionWithStreamKind",
            stage=PassStage.AFTER_INFER_SHAPE,
            kind="stream",
        )


# ── 执行序：同阶段多个资源规划 pass 按 pass 名字典序 ───────────────────


def test_resource_plan_descriptors_are_ordered_by_pass_name():
    for name in ("ZebraStreamPass", "AlphaStreamPass", "MikeStreamPass"):

        @_register(name)
        class _Pass(StreamPass):
            def run(self, graph, context):
                return 0

    stream_names = [
        d.pass_name
        for d in registry.get_registered_passes()
        if d.kind in registry._RESOURCE_PLAN_SPECS
    ]
    assert stream_names == sorted(stream_names)


# ── 规格表本身 ─────────────────────────────────────────────────────────


def test_stream_spec_binds_kind_base_class_and_stage_together():
    """kind / base class / allowed stages / handler key must stay consistent."""

    spec = registry._RESOURCE_PLAN_SPECS["stream"]
    assert spec.kind == "stream"
    assert spec.base_cls is StreamPass
    assert spec.allowed_stages == frozenset({PassStage.AFTER_ASSIGN_LOGIC_STREAM})
    assert spec.handler_key == "stream"
    # Adding a kind must not require touching the decorator itself.
    assert set(registry._RESOURCE_PLAN_SPECS) == {"stream"}
