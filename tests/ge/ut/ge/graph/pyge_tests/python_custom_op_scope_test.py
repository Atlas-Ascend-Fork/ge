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

"""Pytest coverage for model-scoped Python custom op restoration (stage 3).

These tests exercise the pure-Python restore primitives: scope-local capture
of ``register_op_impl``, in-memory source initialization (no disk, no
``sys.path`` change), scoped descriptor lookup used by runtime holder
creation, and scope lifecycle (ids never recycled).  Prototype suppression
(``register_op`` -> declare-only) needs the native graph wrapper and is
covered alongside the C++ restore hook.
"""

import importlib
import sys

import pytest

try:
    custom_op = importlib.import_module("ge.custom_op")
    op_impl_registry = importlib.import_module("ge.custom_op.registry")
    scope_mod = importlib.import_module("ge.custom_op._scope")
except ImportError as exc:  # pragma: no cover - environment without the package
    pytest.skip(f"无法导入 Python custom op 相关模块: {exc}", allow_module_level=True)


# 一段可在内存中 exec 的插件源码：注册一个 portable 实现类。
PORTABLE_SOURCE = b"""
from ge.custom_op import register_op_impl


@register_op_impl(op_type="ScopedPortableOp")
class ScopedPortableOp:
    def serialize(self) -> bytes:
        return b"state"

    def deserialize(self, data: bytes) -> None:
        _ = data
"""

EAGER_SOURCE = b"""
from ge.custom_op import register_op_impl


@register_op_impl(op_type="ScopedEagerOp")
class ScopedEagerOp:
    def execute(self, ctx):
        _ = ctx
"""

# 顶层语句抛异常，用于验证失败回滚（module 与已 capture 的 descriptor 都撤销）。
FAILING_SOURCE = b"""
from ge.custom_op import register_op_impl


@register_op_impl(op_type="ScopedFailsLater")
class ScopedFailsLater:
    def execute(self, ctx):
        _ = ctx


raise RuntimeError("boom during module exec")
"""


def _proto_source(extra_dims: int) -> bytes:
    """声明同一 op_type 但推导逻辑不同的原型（分子放两个模型里比对）。"""
    return (
        "from ge.custom_op import register_op\n"
        "from ge.graph import DataType\n"
        "from ge.runtime import TensorDesc\n"
        "\n"
        "\n"
        '@register_op(op_type="ScopedProtoOp")\n'
        "def infer_meta(x: TensorDesc) -> TensorDesc:\n"
        "    dims = list(x.shape.origin_shape.dims)\n"
        f"    dims[-1] = dims[-1] + {extra_dims}\n"
        "    return TensorDesc(dims, DataType.DT_INT32)\n"
    ).encode()


SYNTAX_ERROR_SOURCE = b"def (:\n"


@pytest.fixture(autouse=True)
def clean_state():
    custom_op.clear_registered_op_impls()
    scope_mod.reset_model_scopes_for_test()
    yield
    scope_mod.reset_model_scopes_for_test()
    custom_op.clear_registered_op_impls()


def test_source_initializes_in_memory_and_captures_into_scope():
    scope = scope_mod.open_model_scope(1)
    module = scope.initialize_source("a" * 64, "plugin.py", PORTABLE_SOURCE)

    # module 进入 sys.modules，名字带 scope_id 前缀；合成文件名不指向文件系统。
    assert module.__name__ in sys.modules
    assert module.__name__ == f"_ge_om_custom_op_1_{'a' * 64}"
    assert module.__file__ == "<ge-om:1/plugin.py>"
    assert not module.__file__.startswith("/")

    # descriptor 进了 scope-local capture。
    collected = scope.collect_impl_descriptors()
    assert len(collected) == 1
    assert collected[0]["op_type"] == "ScopedPortableOp"
    assert collected[0]["class_qualname"] == "ScopedPortableOp"
    assert "portable" in collected[0]["interfaces"]
    # capabilities 由 C++ ParseInterfaces 派生，Python 收集侧不重复实现。
    assert "capabilities" not in collected[0]


def test_global_registry_untouched_during_scope_capture():
    scope = scope_mod.open_model_scope(2)
    scope.initialize_source("b" * 64, "plugin.py", PORTABLE_SOURCE)

    # 进程级全局注册表零写入：scope 内注册的算子对全局不可见。
    global_types = [d.op_type for d in op_impl_registry.get_registered_op_impls()]
    assert "ScopedPortableOp" not in global_types


def test_registration_outside_scope_still_goes_global():
    # 无激活 scope 时，正常注册路径行为不变（写全局注册表）。
    assert scope_mod.get_active_model_scope() is None

    ns = {}
    exec(
        "from ge.custom_op import register_op_impl\n"
        "@register_op_impl(op_type='GlobalOp')\n"
        "class GlobalOp:\n"
        "    def execute(self, ctx):\n"
        "        _ = ctx\n",
        ns,
    )
    global_types = [d.op_type for d in op_impl_registry.get_registered_op_impls()]
    assert "GlobalOp" in global_types


def test_scoped_descriptor_lookup_fallback():
    scope = scope_mod.open_model_scope(3)
    scope.initialize_source("c" * 64, "plugin.py", EAGER_SOURCE)

    collected = scope.collect_impl_descriptors()
    descriptor_key = collected[0]["descriptor_key"]

    # 全局查不到，但 scoped 回退能命中（供运行时 create_op_impl_holder 使用）。
    assert (
        op_impl_registry._OP_IMPL_REGISTRY.get_by_descriptor_key(descriptor_key) is None
    )
    descriptor = op_impl_registry.get_registered_op_impl_by_descriptor_key(
        descriptor_key
    )
    assert descriptor is not None
    assert descriptor.op_type == "ScopedEagerOp"
    # 回退查到的 descriptor 携带真实 cls，可实例化（holder 创建依赖它）。
    instance = descriptor.cls()
    assert isinstance(instance, descriptor.cls)


def test_failed_source_exec_removes_module_and_raises():
    scope = scope_mod.open_model_scope(4)
    with pytest.raises(RuntimeError):
        scope.initialize_source("d" * 64, "plugin.py", FAILING_SOURCE)

    # 失败后 module 从 sys.modules 移除；scope 内已捕获的 descriptor 不做逐项
    # 回滚——加载侧 guard 会关闭整个 scope（Close 统一清理），部分状态不会
    # 被后续查询观测到（R1-5e：删除 watermark/rollback 机制）。
    module_name = f"_ge_om_custom_op_4_{'d' * 64}"
    assert module_name not in sys.modules


def test_syntax_error_source_raises_model_scope_error():
    scope = scope_mod.open_model_scope(5)
    with pytest.raises(scope_mod.ModelScopeError):
        scope.initialize_source("e" * 64, "plugin.py", SYNTAX_ERROR_SOURCE)


def test_source_init_does_not_touch_sys_path(tmp_path):
    before = list(sys.path)
    scope = scope_mod.open_model_scope(6)
    scope.initialize_source("f" * 64, "plugin.py", EAGER_SOURCE)
    assert sys.path == before


def test_reject_empty_or_invalid_source():
    scope = scope_mod.open_model_scope(7)
    # 跨语言边界只校验 source 类型（bytes）；空 source / 空 artifact_id 的不变量
    # 由保存侧 ValidateArtifact 与 C ABI 入口的 sources 配对校验保证（R2-5），
    # 本层不再复校——空 bytes 会作为空模块成功执行，属预期行为。
    with pytest.raises(scope_mod.ModelScopeError):
        scope.initialize_source("0" * 64, "plugin.py", "not-bytes")


def test_duplicate_artifact_in_same_scope_rejected():
    scope = scope_mod.open_model_scope(8)
    scope.initialize_source("1" * 64, "plugin.py", EAGER_SOURCE)
    with pytest.raises(scope_mod.ModelScopeError):
        scope.initialize_source("1" * 64, "plugin.py", EAGER_SOURCE)


def test_scope_id_never_recycled_and_must_increase():
    scope_mod.open_model_scope(10)
    # 已用过的 id 不能重开（即便对应 scope 已关闭）。
    scope_mod.close_model_scope(10)
    with pytest.raises(scope_mod.ModelScopeError):
        scope_mod.open_model_scope(10)
    # 必须单调递增：小于历史最高 id 的都被拒。
    with pytest.raises(scope_mod.ModelScopeError):
        scope_mod.open_model_scope(5)
    # 更大的 id 可以开。
    scope = scope_mod.open_model_scope(11)
    assert scope.scope_id == 11


def test_scope_id_validation():
    with pytest.raises(scope_mod.ModelScopeError):
        scope_mod.open_model_scope(0)
    with pytest.raises(scope_mod.ModelScopeError):
        scope_mod.open_model_scope(-1)
    with pytest.raises(scope_mod.ModelScopeError):
        scope_mod.open_model_scope(True)  # bool 不是合法 scope id


def test_close_is_idempotent_and_drops_modules():
    scope = scope_mod.open_model_scope(20)
    module = scope.initialize_source("2" * 64, "plugin.py", EAGER_SOURCE)
    module_name = module.__name__
    assert module_name in sys.modules

    assert scope_mod.close_model_scope(20) is True
    assert module_name not in sys.modules
    # 幂等：再关一次返回 False（未知/已退休），不抛异常。
    assert scope_mod.close_model_scope(20) is False


def test_capture_after_close_rejected():
    scope = scope_mod.open_model_scope(21)
    scope_mod.close_model_scope(21)
    with pytest.raises(scope_mod.ModelScopeError):
        scope.initialize_source("3" * 64, "plugin.py", EAGER_SOURCE)


def test_get_active_scope_is_none_outside_source_init():
    # initialize_source 只在 exec 期间激活 scope；返回后当前上下文无激活 scope。
    scope = scope_mod.open_model_scope(30)
    scope.initialize_source("4" * 64, "plugin.py", EAGER_SOURCE)
    assert scope_mod.get_active_model_scope() is None


def test_find_scoped_impl_descriptor_unknown_returns_none():
    assert scope_mod.find_scoped_impl_descriptor("no:such:key") is None


# ---------------------------------------------------------------------------
# canonical IR 快照注入与查询（保存侧冻结，加载态 D4 兜底）
# ---------------------------------------------------------------------------


def _sample_ir_defs():
    return {
        "ScopedIrOp": {
            "inputs": [{"name": "x", "kind": 0}, {"name": "y", "kind": 0}],
            "attrs": [{"name": "factor", "type": "float"}],
            "outputs": [{"name": "z", "kind": 0}],
        }
    }


def test_initialize_source_records_ir_defs():
    scope = scope_mod.open_model_scope(40)
    scope.initialize_source("5" * 64, "plugin.py", EAGER_SOURCE, _sample_ir_defs())
    assert scope.get_ir_meta_by_op_type("ScopedIrOp") == _sample_ir_defs()["ScopedIrOp"]
    # 未注入的 op_type 查询返回 None
    assert scope.get_ir_meta_by_op_type("UnknownOp") is None


def test_find_scoped_ir_meta_spans_live_scopes():
    scope = scope_mod.open_model_scope(41)
    scope.initialize_source("6" * 64, "plugin.py", EAGER_SOURCE, _sample_ir_defs())
    assert scope_mod.find_scoped_ir_meta("ScopedIrOp") is not None
    assert scope_mod.find_scoped_ir_meta("UnknownOp") is None


def test_find_scoped_ir_meta_rejects_conflicting_ir_across_models():
    # 两个模型并存、同一 op_type 冻结了不同 IR：按 op_type 首个命中即返回会让后
    # 加载的模型按前一个模型的 schema 绑定参数（静默参数错位）。必须点名失败。
    other_ir_defs = {
        "ScopedIrOp": {
            "inputs": [{"name": "x", "kind": 0}],
            "attrs": [],
            "outputs": [{"name": "z", "kind": 0}],
        }
    }
    scope_mod.open_model_scope(45).initialize_source(
        "a" * 64, "plugin.py", EAGER_SOURCE, _sample_ir_defs()
    )
    scope_mod.open_model_scope(46).initialize_source(
        "b" * 64, "plugin.py", EAGER_SOURCE, other_ir_defs
    )
    with pytest.raises(
        scope_mod.ModelScopeError, match="conflicting canonical IR.*ScopedIrOp"
    ):
        scope_mod.find_scoped_ir_meta("ScopedIrOp")


def test_find_scoped_ir_meta_accepts_same_source_across_models():
    # 同一份源码（同一 artifact_id）被两个模型加载是正常形态，不得误报冲突。
    scope_mod.open_model_scope(47).initialize_source(
        "c" * 64, "plugin.py", EAGER_SOURCE, _sample_ir_defs()
    )
    scope_mod.open_model_scope(48).initialize_source(
        "c" * 64, "plugin.py", EAGER_SOURCE, _sample_ir_defs()
    )
    assert (
        scope_mod.find_scoped_ir_meta("ScopedIrOp") == _sample_ir_defs()["ScopedIrOp"]
    )


def test_find_scoped_proto_rejects_conflicting_declarations_across_models():
    # 两个模型并存、同一 op_type 声明了不同的 infer_func：取错就是静默算错 shape。
    scope_mod.open_model_scope(49).initialize_source(
        "e" * 64, "proto_a.py", _proto_source(2)
    )
    scope_mod.open_model_scope(50).initialize_source(
        "f" * 64, "proto_b.py", _proto_source(3)
    )
    with pytest.raises(
        scope_mod.ModelScopeError,
        match="conflicting prototype declaration.*ScopedProtoOp",
    ):
        scope_mod.find_scoped_proto("ScopedProtoOp")


def test_find_scoped_proto_accepts_same_source_across_models():
    # 同一份源码（同一 artifact_id）被两个模型加载 → 声明同源，不得误报。
    scope_mod.open_model_scope(51).initialize_source(
        "1" * 64, "proto.py", _proto_source(2)
    )
    scope_mod.open_model_scope(52).initialize_source(
        "1" * 64, "proto.py", _proto_source(2)
    )
    proto = scope_mod.find_scoped_proto("ScopedProtoOp")
    assert proto is not None
    assert proto.infer_func is not None


def test_failed_source_exec_keeps_ir_defs_until_scope_close():
    scope = scope_mod.open_model_scope(42)
    with pytest.raises(RuntimeError, match="boom during module exec"):
        scope.initialize_source(
            "7" * 64, "plugin.py", FAILING_SOURCE, _sample_ir_defs()
        )
    # 执行失败：IR 注入不逐项回滚（scope 将随加载失败被整体 Close 清理）；
    # close 之后所有 scope 内状态（IR/descriptor/module）一并清空。
    assert scope_mod.close_model_scope(42) is True
    assert scope_mod.find_scoped_ir_meta("ScopedIrOp") is None


def test_close_drops_ir_defs():
    scope = scope_mod.open_model_scope(43)
    scope.initialize_source("8" * 64, "plugin.py", EAGER_SOURCE, _sample_ir_defs())
    assert scope_mod.close_model_scope(43) is True
    assert scope_mod.find_scoped_ir_meta("ScopedIrOp") is None


def test_inconsistent_ir_across_artifacts_rejected():
    plain_source = b"value = 1\n"
    scope = scope_mod.open_model_scope(44)
    scope.initialize_source("9" * 64, "first.py", plain_source, _sample_ir_defs())
    # 同一 op_type、不同 IR：说明保存侧冻结逻辑内部不一致，硬失败
    conflicting = {
        "ScopedIrOp": {
            "inputs": [{"name": "x", "kind": 0}],
            "attrs": [],
            "outputs": [{"name": "z", "kind": 0}],
        }
    }
    with pytest.raises(scope_mod.ModelScopeError):
        scope.initialize_source("a" * 64, "second.py", plain_source, conflicting)


def test_duplicate_identical_ir_across_artifacts_accepted():
    # 不注册任何实现的空源码：本测试只关心 IR 去重语义
    plain_source = b"value = 1\n"
    scope = scope_mod.open_model_scope(45)
    scope.initialize_source("b" * 64, "first.py", plain_source, _sample_ir_defs())
    # 同一 op_type、相同 IR（重复保存的 artifact）：允许
    scope.initialize_source("c" * 64, "second.py", plain_source, _sample_ir_defs())
    assert scope.get_ir_meta_by_op_type("ScopedIrOp") is not None
