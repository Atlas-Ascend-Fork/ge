#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# -----------------------------------------------------------------------------------------------------------
# Copyright (c) 2026 Huawei Technologies Co., Ltd.
# This program is free software, you can redistribute it and/or modify it under the terms and conditions of
# CANN Open Software License Agreement Version 2.0 (the "License").
# Please refer to the License for details. You may not use this file except in compliance with the License.
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
# INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
# See LICENSE in the root of the software repository for the full text of the License.
# -----------------------------------------------------------------------------------------------------------

"""Pytest coverage for Python custom op deployment artifact freezing."""

import hashlib
import importlib
import importlib.util
import sys

import pytest

try:
    bootstrap = importlib.import_module("ge.custom_op.bootstrap")
    custom_op = importlib.import_module("ge.custom_op")
    deployment = importlib.import_module("ge.custom_op._deployment")
    op_impl_registry = importlib.import_module("ge.custom_op.registry")
    proto_registry = importlib.import_module("ge.custom_op.proto")
except ImportError as exc:
    pytest.skip(f"无法导入 Python custom op 相关模块: {exc}", allow_module_level=True)


@pytest.fixture(autouse=True)
def clean_registry_and_artifacts():
    custom_op.clear_registered_op_impls()
    proto_registry.clear_registered_op_protos()
    deployment.clear_frozen_artifacts()
    yield
    deployment.clear_frozen_artifacts()
    proto_registry.clear_registered_op_protos()
    custom_op.clear_registered_op_impls()


def _load_plugin(path, name):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    try:
        spec.loader.exec_module(module)
    except BaseException:
        del sys.modules[name]
        raise
    return module


def _write_plugin(tmp_path, file_name, body):
    path = tmp_path / file_name
    path.write_text(body, encoding="utf-8")
    return path


PORTABLE_BODY = """
from ge.custom_op import register_op_impl


@register_op_impl(op_type="DeployPortableOp")
class DeployPortableOp:
    def serialize(self) -> bytes:
        return b"state"

    def deserialize(self, data: bytes) -> None:
        _ = data
"""

EAGER_ONLY_BODY = """
from ge.custom_op import register_op_impl


@register_op_impl(op_type="DeployEagerOnlyOp")
class DeployEagerOnlyOp:
    def execute(self, ctx):
        _ = ctx
"""

MIXED_BODY = """
from ge.custom_op import register_op_impl


@register_op_impl(op_type="DeployMixedEagerOp")
class DeployMixedEagerOp:
    def execute(self, ctx):
        _ = ctx


@register_op_impl(op_type="DeployMixedPortableOp")
class DeployMixedPortableOp:
    def serialize(self) -> bytes:
        return b"state"

    def deserialize(self, data: bytes) -> None:
        _ = data
"""


def test_capability_bits_match_cpp_capability_header():
    # 与 inc/graph_metadef/graph/custom_op/capability.h 的位定义逐一对齐
    assert deployment.CAPABILITY_EAGER_EXECUTE == 1 << 0
    assert deployment.CAPABILITY_COMPILABLE == 1 << 1
    assert deployment.CAPABILITY_SHAPE_INFER == 1 << 2
    assert deployment.CAPABILITY_PORTABLE == 1 << 3
    assert deployment.CAPABILITY_ARGS_UPDATER == 1 << 4
    assert deployment.CAPABILITY_ANNOTATED_ARGS == 1 << 5
    assert deployment.CAPABILITY_INFER_META == 1 << 6
    assert deployment.CAPABILITY_HOST_CPU_EXECUTE == 1 << 7
    assert deployment.CAPABILITY_RESOURCE_REPORTING == 1 << 8


def test_capabilities_from_interfaces_mirrors_parse_interfaces():
    # 与 python_custom_op_bridge_descriptors.cc 的 ParseInterfaces 映射一致
    assert deployment.capabilities_from_interfaces(["eager_execute"]) == 1 << 0
    assert deployment.capabilities_from_interfaces(["host_cpu_execute"]) == 1 << 7
    assert deployment.capabilities_from_interfaces(["compilable"]) == 1 << 1
    assert deployment.capabilities_from_interfaces(["annotated_args"]) == 1 << 5
    assert deployment.capabilities_from_interfaces(["portable"]) == 1 << 3
    assert deployment.capabilities_from_interfaces([]) == 0
    combined = deployment.capabilities_from_interfaces(
        ["eager_execute", "annotated_args", "portable"]
    )
    assert combined == (1 << 0) | (1 << 5) | (1 << 3)


def test_capabilities_from_interfaces_rejects_unknown_interface():
    with pytest.raises(deployment.DeploymentArtifactError):
        deployment.capabilities_from_interfaces(["not_an_interface"])


def test_freeze_includes_files_without_portable_impl(tmp_path):
    # 无条件冻结：源码可定位的 eager-only 实现同样冻结成 artifact（是否进 OM
    # 由保存侧按图使用决定），对齐 C++ 自定义算子无条件收 SO 的模型。
    path = _write_plugin(tmp_path, "eager_only_plugin.py", EAGER_ONLY_BODY)
    _load_plugin(path, "_ge_py_custom_op_deploy_eager_only")

    artifacts = deployment.freeze_registered_artifacts(
        op_impl_registry.get_registered_op_impls()
    )

    assert len(artifacts) == 1
    assert [item.op_type for item in artifacts[0].descriptors] == ["DeployEagerOnlyOp"]


def test_freeze_captures_source_bytes_and_digest(tmp_path):
    path = _write_plugin(tmp_path, "portable_plugin.py", PORTABLE_BODY)
    _load_plugin(path, "_ge_py_custom_op_deploy_portable")

    artifacts = deployment.freeze_registered_artifacts(
        op_impl_registry.get_registered_op_impls()
    )

    assert len(artifacts) == 1
    artifact = artifacts[0]
    raw = path.read_bytes()
    assert artifact.source == raw
    assert artifact.source_sha256 == hashlib.sha256(raw).digest()
    assert artifact.artifact_id == hashlib.sha256(raw).hexdigest()
    assert len(artifact.artifact_id) == 64
    assert artifact.artifact_id == artifact.artifact_id.lower()


def test_source_name_is_basename_and_leaks_no_build_path(tmp_path):
    path = _write_plugin(tmp_path, "portable_plugin.py", PORTABLE_BODY)
    _load_plugin(path, "_ge_py_custom_op_deploy_portable_name")

    artifact = deployment.freeze_registered_artifacts(
        op_impl_registry.get_registered_op_impls()
    )[0]

    assert artifact.source_name == "portable_plugin.py"
    assert str(tmp_path) not in artifact.source_name
    assert "/" not in artifact.source_name


def test_freeze_includes_all_descriptors_of_a_portable_file(tmp_path):
    path = _write_plugin(tmp_path, "mixed_plugin.py", MIXED_BODY)
    _load_plugin(path, "_ge_py_custom_op_deploy_mixed")

    artifact = deployment.freeze_registered_artifacts(
        op_impl_registry.get_registered_op_impls()
    )[0]

    # 同一文件内的非 portable 实现也必须进入 manifest，加载侧要恢复该文件产生的全部 creator
    assert [item.op_type for item in artifact.descriptors] == [
        "DeployMixedEagerOp",
        "DeployMixedPortableOp",
    ]
    by_op_type = {item.op_type: item for item in artifact.descriptors}
    assert by_op_type["DeployMixedEagerOp"].capabilities == 1 << 0
    assert by_op_type["DeployMixedPortableOp"].capabilities == 1 << 3


def test_descriptors_are_sorted_by_op_type_then_class_name(tmp_path):
    body = """
from ge.custom_op import register_op_impl


@register_op_impl(op_type="Zebra")
class Zebra:
    def serialize(self) -> bytes:
        return b"z"

    def deserialize(self, data: bytes) -> None:
        _ = data


@register_op_impl(op_type="Apple")
class Apple:
    def serialize(self) -> bytes:
        return b"a"

    def deserialize(self, data: bytes) -> None:
        _ = data
"""
    path = _write_plugin(tmp_path, "sorted_plugin.py", body)
    _load_plugin(path, "_ge_py_custom_op_deploy_sorted")

    artifact = deployment.freeze_registered_artifacts(
        op_impl_registry.get_registered_op_impls()
    )[0]

    assert [item.op_type for item in artifact.descriptors] == ["Apple", "Zebra"]


def test_identical_content_in_two_files_shares_one_artifact_id(tmp_path):
    first = _write_plugin(tmp_path, "first_plugin.py", PORTABLE_BODY)
    second = _write_plugin(tmp_path, "second_plugin.py", PORTABLE_BODY)
    _load_plugin(first, "_ge_py_custom_op_deploy_first")
    deployment.clear_frozen_artifacts()
    custom_op.clear_registered_op_impls()
    _load_plugin(second, "_ge_py_custom_op_deploy_second")

    first_artifacts = deployment.freeze_registered_artifacts(
        op_impl_registry.get_registered_op_impls()
    )
    assert len(first_artifacts) == 1

    # 内容相同的两个文件摘要一致，这是跨 artifact 去重的基础
    deployment.clear_frozen_artifacts()
    custom_op.clear_registered_op_impls()
    _load_plugin(first, "_ge_py_custom_op_deploy_first_again")
    second_artifacts = deployment.freeze_registered_artifacts(
        op_impl_registry.get_registered_op_impls()
    )
    assert second_artifacts[0].artifact_id == first_artifacts[0].artifact_id


def test_class_name_uses_qualname_for_nested_class(tmp_path):
    body = """
from ge.custom_op import register_op_impl


class Outer:
    @register_op_impl(op_type="DeployNestedPortableOp")
    class Nested:
        def serialize(self) -> bytes:
            return b"nested"

        def deserialize(self, data: bytes) -> None:
            _ = data
"""
    path = _write_plugin(tmp_path, "nested_plugin.py", body)
    _load_plugin(path, "_ge_py_custom_op_deploy_nested")

    artifact = deployment.freeze_registered_artifacts(
        op_impl_registry.get_registered_op_impls()
    )[0]

    assert artifact.descriptors[0].class_name == "Outer.Nested"


def test_resolve_source_file_rejects_dynamically_created_class():
    namespace = {}
    exec("class DynamicPortable:\n    pass\n", namespace)
    with pytest.raises(deployment.DeploymentArtifactError):
        deployment.resolve_source_file(namespace["DynamicPortable"])


def test_resolve_source_file_rejects_non_py_suffix(tmp_path):
    path = tmp_path / "plugin.pyc"
    path.write_bytes(b"\x00")

    class FakeCls:
        pass

    FakeCls.__module__ = __name__
    fake_module = importlib.util.module_from_spec(
        importlib.util.spec_from_loader("fake_pyc_module", loader=None)
    )
    fake_module.__file__ = str(path)
    sys.modules["fake_pyc_module"] = fake_module
    try:
        FakeCls.__module__ = "fake_pyc_module"
        with pytest.raises(deployment.DeploymentArtifactError):
            deployment.resolve_source_file(FakeCls)
    finally:
        del sys.modules["fake_pyc_module"]


def test_freeze_is_cached_and_ignores_later_file_changes(tmp_path):
    path = _write_plugin(tmp_path, "cached_plugin.py", PORTABLE_BODY)
    _load_plugin(path, "_ge_py_custom_op_deploy_cached")
    descriptors = op_impl_registry.get_registered_op_impls()

    first = deployment.freeze_registered_artifacts(descriptors)
    original_id = first[0].artifact_id

    # 冻结后修改文件，当前编译仍必须使用已冻结的 bytes
    path.write_text(PORTABLE_BODY + "\n# mutated after freeze\n", encoding="utf-8")
    second = deployment.freeze_registered_artifacts(descriptors)

    assert second[0].artifact_id == original_id
    assert second[0].source == first[0].source


# 回归：LoadPythonCustomOpsIfNeeded 支持运行期间发现新插件。冻结按文件缓存——
# 新文件正常冻结，既有文件复用已冻结 bytes（TOCTOU 仍成立），不做整集缓存。
def test_freeze_picks_up_newly_discovered_plugin_after_first_freeze(tmp_path):
    first_path = _write_plugin(tmp_path, "first_plugin.py", PORTABLE_BODY)
    _load_plugin(first_path, "_ge_py_custom_op_deploy_dynamic_first")

    first = deployment.freeze_registered_artifacts(
        op_impl_registry.get_registered_op_impls()
    )
    assert [item.artifact_id for item in first] == [
        hashlib.sha256(first_path.read_bytes()).hexdigest()
    ]

    # 模拟运行期间发现新插件：注册第二个文件后再冻结
    second_path = _write_plugin(tmp_path, "second_plugin.py", MIXED_BODY)
    _load_plugin(second_path, "_ge_py_custom_op_deploy_dynamic_second")
    # 冻结后篡改第一个文件，验证它复用的是已冻结 bytes 而不是重新读取
    first_path.write_text(
        PORTABLE_BODY + "\n# mutated after freeze\n", encoding="utf-8"
    )

    second = deployment.freeze_registered_artifacts(
        op_impl_registry.get_registered_op_impls()
    )

    ids = {item.artifact_id for item in second}
    assert len(second) == 2
    assert hashlib.sha256(second_path.read_bytes()).hexdigest() in ids
    # 第一个文件复用缓存：id 不变，且不含篡改内容
    assert first[0].artifact_id in ids
    reused = next(item for item in second if item.artifact_id == first[0].artifact_id)
    assert b"mutated after freeze" not in reused.source
    assert b"mutated after freeze" not in second[0].source


def test_freeze_rejects_empty_source_file(tmp_path):
    path = _write_plugin(tmp_path, "empty_plugin.py", PORTABLE_BODY)
    _load_plugin(path, "_ge_py_custom_op_deploy_empty")
    descriptors = op_impl_registry.get_registered_op_impls()
    path.write_bytes(b"")

    with pytest.raises(deployment.DeploymentArtifactError):
        deployment.freeze_registered_artifacts(descriptors)


def test_to_bridge_dict_shape(tmp_path):
    path = _write_plugin(tmp_path, "bridge_dict_plugin.py", PORTABLE_BODY)
    _load_plugin(path, "_ge_py_custom_op_deploy_bridge_dict")

    artifact = deployment.freeze_registered_artifacts(
        op_impl_registry.get_registered_op_impls()
    )[0]
    payload = artifact.to_bridge_dict()

    assert set(payload) == {
        "artifact_id",
        "source_name",
        "source_sha256",
        "source",
        "producer_python_major",
        "producer_python_minor",
        "descriptors",
    }
    assert isinstance(payload["source_sha256"], bytes)
    assert isinstance(payload["source"], bytes)
    assert payload["producer_python_major"] == sys.version_info.major
    assert payload["producer_python_minor"] == sys.version_info.minor
    assert payload["descriptors"][0] == {
        "op_type": "DeployPortableOp",
        "class_name": "DeployPortableOp",
        "capabilities": 1 << 3,
    }
    # descriptor_key 与 module_name 跨进程不稳定，禁止出现在 bridge 载荷中
    assert "descriptor_key" not in payload["descriptors"][0]
    assert "module_name" not in payload["descriptors"][0]


# 回归：冻结的分组阶段曾对所有 descriptor 解析源码文件（先于 portable 过滤），
# exec/动态生成的 eager-only 类会因此抛错并使整个 Python 自定义算子加载失败，
# 破坏"未声明 portable 的插件不受影响"的承诺。
def test_freeze_ignores_unresolvable_eager_only_impl(tmp_path):
    namespace = {}
    exec(
        "from ge.custom_op import register_op_impl\n"
        "\n"
        "@register_op_impl(op_type='DeployDynamicEagerOnlyOp')\n"
        "class DynamicEagerOnlyOp:\n"
        "    def execute(self, ctx):\n"
        "        _ = ctx\n",
        namespace,
    )
    portable_path = _write_plugin(
        tmp_path, "portable_alongside_dynamic.py", PORTABLE_BODY
    )
    _load_plugin(portable_path, "_ge_py_custom_op_deploy_alongside_dynamic")

    artifacts = deployment.freeze_registered_artifacts(
        op_impl_registry.get_registered_op_impls()
    )

    # 动态类不可定位但不声明 portable：不致命，正常冻结可定位的 portable 文件
    assert len(artifacts) == 1
    assert (
        artifacts[0].artifact_id
        == hashlib.sha256(portable_path.read_bytes()).hexdigest()
    )
    assert [item.op_type for item in artifacts[0].descriptors] == ["DeployPortableOp"]


def test_freeze_returns_empty_when_only_unresolvable_impls_registered():
    namespace = {}
    exec(
        "from ge.custom_op import register_op_impl\n"
        "\n"
        "@register_op_impl(op_type='DeployDynamicOnlyOp')\n"
        "class DynamicOnlyOp:\n"
        "    def execute(self, ctx):\n"
        "        _ = ctx\n",
        namespace,
    )

    # 全部实现都不可定位且无 portable：不产生任何 artifact，也不得抛错
    assert (
        deployment.freeze_registered_artifacts(
            op_impl_registry.get_registered_op_impls()
        )
        == []
    )


def _get_registered_impls():
    return op_impl_registry.get_registered_op_impls()


# 原型声明若与实现同文件，执行该文件时 register_op 自然重放，无需独立 artifact；
# 若分文件，则必须独立冻结成 artifact —— 否则部署环境根本没有声明可重放，
# infer_func 永远恢复不出来（能力位补了也调不通）。
SPLIT_PROTO_BODY = """
from ge.custom_op import register_op
from ge.runtime import TensorDesc


@register_op(op_type="DeploySplitProtoOp")
def infer_meta(x: TensorDesc) -> TensorDesc:
    return x
"""

SPLIT_IMPL_BODY = """
from ge.custom_op import register_op_impl


@register_op_impl(op_type="DeploySplitProtoOp")
class DeploySplitProtoOp:
    def execute(self, ctx):
        _ = ctx
"""

SAME_FILE_PROTO_IMPL_BODY = """
from ge.custom_op import register_op, register_op_impl
from ge.runtime import TensorDesc


@register_op(op_type="DeploySameFileProtoOp")
def infer_meta(x: TensorDesc) -> TensorDesc:
    return x


@register_op_impl(op_type="DeploySameFileProtoOp")
class DeploySameFileProtoOp:
    def execute(self, ctx):
        _ = ctx
"""


def test_freeze_gives_split_file_prototype_its_own_artifact(tmp_path):
    proto_path = _write_plugin(tmp_path, "split_proto_plugin.py", SPLIT_PROTO_BODY)
    impl_path = _write_plugin(tmp_path, "split_impl_plugin.py", SPLIT_IMPL_BODY)
    _load_plugin(proto_path, "_ge_py_custom_op_deploy_split_proto")
    _load_plugin(impl_path, "_ge_py_custom_op_deploy_split_impl")

    artifacts = deployment.freeze_registered_artifacts(
        op_impl_registry.get_registered_op_impls(),
        proto_registry.get_registered_op_protos(),
    )

    # 两个 artifact：实现文件（实现身份）与原型文件（声明），op_type 相同
    assert len(artifacts) == 2
    by_source = {item.source_name: item for item in artifacts}
    assert set(by_source) == {"split_proto_plugin.py", "split_impl_plugin.py"}
    proto_descriptor = by_source["split_proto_plugin.py"].descriptors[0]
    assert proto_descriptor.op_type == "DeploySplitProtoOp"
    assert proto_descriptor.capabilities == deployment._PROTO_CAPABILITIES
    impl_descriptor = by_source["split_impl_plugin.py"].descriptors[0]
    assert impl_descriptor.op_type == "DeploySplitProtoOp"
    assert impl_descriptor.capabilities == deployment.CAPABILITY_EAGER_EXECUTE


def test_freeze_does_not_duplicate_same_file_prototype(tmp_path):
    path = _write_plugin(
        tmp_path, "same_file_proto_plugin.py", SAME_FILE_PROTO_IMPL_BODY
    )
    _load_plugin(path, "_ge_py_custom_op_deploy_same_file")

    artifacts = deployment.freeze_registered_artifacts(
        op_impl_registry.get_registered_op_impls(),
        proto_registry.get_registered_op_protos(),
    )

    # 同文件：执行该文件时 register_op 自然重放，不得再产出一条原型条目
    assert len(artifacts) == 1
    assert [item.op_type for item in artifacts[0].descriptors] == [
        "DeploySameFileProtoOp"
    ]
    assert artifacts[0].descriptors[0].capabilities == (
        deployment.CAPABILITY_EAGER_EXECUTE
    )


def test_freeze_silently_skips_unresolvable_prototype():
    namespace = {}
    exec(
        "from ge.custom_op import register_op\n"
        "from ge.runtime import TensorDesc\n"
        "\n"
        "@register_op(op_type='DeployDynamicProtoOp')\n"
        "def infer_meta(x: TensorDesc) -> TensorDesc:\n"
        "    return x\n",
        namespace,
    )

    # 声明不可定位：与实现侧同策略静默跳过，不阻断冻结
    assert (
        deployment.freeze_registered_artifacts(
            op_impl_registry.get_registered_op_impls(),
            proto_registry.get_registered_op_protos(),
        )
        == []
    )


def test_freeze_warns_when_prototype_source_is_not_packed(caplog):
    """原型定义文件未随任何 artifact 打包时必须给出一条提示。

    判据是「已声明原型的 op_type」与「已冻结 artifact 覆盖的 op_type」的差集：
    同文件形态下 op_type 必然落在已打包集合里，不会误报；只有原型源码真的没进
    OM 时才进差集——那正是部署进程静默失去 shape 推导能力的那一类。
    """

    namespace = {}
    exec(
        "from ge.custom_op import register_op\n"
        "from ge.runtime import TensorDesc\n"
        "\n"
        "@register_op(op_type='DeployUnpackedProtoOp')\n"
        "def infer_meta(x: TensorDesc) -> TensorDesc:\n"
        "    return x\n",
        namespace,
    )

    with caplog.at_level("WARNING"):
        bootstrap.freeze_deployment_artifacts()

    assert any(
        "DeployUnpackedProtoOp" in record.getMessage()
        and record.levelname == "WARNING"
        and "is not packed into the OM" in record.getMessage()
        for record in caplog.records
    )


def test_freeze_does_not_warn_for_packed_prototype(tmp_path, caplog):
    """同文件原型随实现文件一起打包，差集为空，不应产生误报。"""

    path = _write_plugin(tmp_path, "packed_plugin.py", SAME_FILE_PROTO_IMPL_BODY)
    _load_plugin(path, "_ge_py_custom_op_deploy_packed_proto")

    with caplog.at_level("WARNING"):
        artifacts = bootstrap.freeze_deployment_artifacts()

    assert len(artifacts) == 1
    assert not any(
        "is not packed into the OM" in record.getMessage() for record in caplog.records
    )
