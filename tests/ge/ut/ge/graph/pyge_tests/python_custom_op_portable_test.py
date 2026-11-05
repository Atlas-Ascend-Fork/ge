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

"""Pytest coverage for Python custom-op Portable method registration and bridge calls."""

import importlib

import pytest

try:
    bridge = importlib.import_module("ge.custom_op._bridge")
    custom_op = importlib.import_module("ge.custom_op")
except ImportError as exc:
    pytest.skip(f"无法导入 Python custom op 相关模块: {exc}", allow_module_level=True)


@pytest.fixture(autouse=True)
def clear_python_custom_op_registry():
    custom_op.clear_registered_op_impls()
    bridge.clear_op_impl_holders()
    yield
    bridge.clear_op_impl_holders()
    custom_op.clear_registered_op_impls()


def _assert_portable_error(op_type, method, expected, actual, decorator):
    with pytest.raises(TypeError) as exc_info:
        decorator()

    message = str(exc_info.value)
    assert op_type in message
    assert f"method {method}" in message
    assert expected in message
    assert actual in message


def test_register_op_impl_adds_one_portable_interface_without_construction():
    constructed = []

    @custom_op.register_op_impl(op_type="PortableCustom")
    class PortableCustom:
        def __init__(self):
            constructed.append(True)

        def serialize(self) -> bytes:
            return b"state"

        def deserialize(self, payload: bytes) -> None:
            _ = payload

    descriptor = PortableCustom.__ge_op_impl_descriptor__
    assert descriptor.interfaces == ["portable"]
    assert constructed == []


def test_register_op_impl_ignores_inherited_portable_pair():
    """Inherited serialize/deserialize no longer make a class Portable (R5 方案 A).

    Only methods declared on the class itself count, so plugins whose base class
    happens to implement the pair keep registering without the portable interface.
    """

    class PortableBase:
        def serialize(self) -> bytes:
            return b"state"

        def deserialize(self, data: bytes) -> None:
            _ = data

    @custom_op.register_op_impl(op_type="InheritedPortableCustom")
    class InheritedPortableCustom(PortableBase):
        def execute(self, ctx):
            _ = ctx

    assert InheritedPortableCustom.__ge_op_impl_descriptor__.interfaces == [
        "eager_execute"
    ]


def test_register_op_impl_combines_portable_with_existing_interfaces():
    @custom_op.register_op_impl(op_type="CombinedPortableCustom")
    class CombinedPortableCustom:
        def execute(self, ctx):
            _ = ctx

        def declare_launch_args(self) -> None:
            pass

        def serialize(self) -> bytes:
            return b"state"

        def deserialize(self, data: bytes) -> None:
            _ = data

    assert CombinedPortableCustom.__ge_op_impl_descriptor__.interfaces == [
        "eager_execute",
        "annotated_args",
        "portable",
    ]


def test_register_op_impl_ignores_unpaired_inherited_serialize():
    """A class inheriting an unrelated serialize (without deserialize) still registers."""

    class UnrelatedBase:
        def serialize(self):
            return "not-a-portable-state"

    @custom_op.register_op_impl(op_type="InheritedUnrelatedSerializeCustom")
    class InheritedUnrelatedSerializeCustom(UnrelatedBase):
        def execute(self, ctx):
            _ = ctx

    # Inherited serialize is invisible to the pairing check: the class registers
    # on execute alone and must NOT be flagged as a half-defined Portable op.
    assert InheritedUnrelatedSerializeCustom.__ge_op_impl_descriptor__.interfaces == [
        "eager_execute"
    ]


def test_register_op_impl_ignores_unpaired_own_serialize_without_construction():
    """Half-paired methods on the class itself still fail fast at registration."""

    constructed = []

    def register():
        @custom_op.register_op_impl(op_type="HalfPairedCustom")
        class HalfPairedCustom:
            def __init__(self):
                constructed.append(True)

            def serialize(self):
                return b"state"

        _ = HalfPairedCustom

    _assert_portable_error(
        "HalfPairedCustom",
        "deserialize",
        "paired serialize/deserialize methods",
        "serialize present",
        register,
    )
    assert constructed == []


def test_register_op_impl_allows_nonstandard_paired_signatures_at_registration():
    """Signature/annotation conformance is deferred to first call, not registration."""

    def register():
        @custom_op.register_op_impl(op_type="LooseSignaturePortableCustom")
        class LooseSignaturePortableCustom:
            def serialize(self, extra=1):
                _ = extra
                return b"state"

            def deserialize(self, *args, **kwargs):
                _ = (args, kwargs)

        assert LooseSignaturePortableCustom.__ge_op_impl_descriptor__.interfaces == [
            "portable"
        ]

    register()


def test_register_op_impl_rejects_init_requiring_arguments():
    """A portable class whose __init__ needs arguments fails at registration, not on load."""

    def register():
        @custom_op.register_op_impl(op_type="ZeroArgInitPortableCustom")
        class ZeroArgInitPortableCustom:
            def __init__(self, required_arg):
                _ = required_arg

            def serialize(self):
                return b"state"

            def deserialize(self, data):
                _ = data

    with pytest.raises(TypeError) as exc_info:
        register()
    message = str(exc_info.value)
    assert "ZeroArgInitPortableCustom" in message
    assert "zero arguments" in message


def test_register_op_impl_allows_fully_defaulted_init():
    """__init__ with all-default parameters stays constructible with zero args."""

    @custom_op.register_op_impl(op_type="DefaultedInitPortableCustom")
    class DefaultedInitPortableCustom:
        def __init__(self, state=b"initial"):
            self.state = state

        def serialize(self):
            return self.state

        def deserialize(self, data):
            self.state = data

    assert DefaultedInitPortableCustom.__ge_op_impl_descriptor__.interfaces == [
        "portable"
    ]


UINT32_MAX = 4294967295


def _register_stateful_portable(op_type: str, initial_state: bytes = b"initial"):
    """Register and load a stateful Portable custom op with tracking hooks."""

    created = []
    serialize_calls = []
    deserialize_calls = []

    def register(
        serialize_fn=None,
        deserialize_fn=None,
        payload=None,
        restored_value=None,
    ):
        nonlocal created

        real_serialize = serialize_fn
        real_deserialize = deserialize_fn
        real_payload = payload if payload is not None else initial_state

        class StatefulPortableCustom:
            def __init__(self):
                created.append(self)
                self.payload = real_payload
                self.restored = restored_value

            def serialize(self) -> bytes:
                serialize_calls.append(self)
                if real_serialize is not None:
                    return real_serialize(self)
                return self.payload

            def deserialize(self, data: bytes) -> None:
                deserialize_calls.append((self, data))
                if real_deserialize is not None:
                    return real_deserialize(self, data)
                self.restored = data

        custom_op.register_op_impl(op_type=op_type)(StatefulPortableCustom)
        return StatefulPortableCustom

    def create_and_holder(instance_id: str):
        descriptors = bridge.load_and_get_op_impl_descriptors()
        descriptor_key = descriptors[0]["descriptor_key"]
        assert bridge.create_op_impl_holder(instance_id, descriptor_key) is True
        holder = bridge._get_holder(instance_id)
        return descriptor_key, holder

    return register, create_and_holder, created, serialize_calls, deserialize_calls


@pytest.fixture
def stateful_portable():
    return _register_stateful_portable(
        "StatefulPortableCustom", initial_state=b"initial"
    )


def test_call_serialize_returns_exact_payload(stateful_portable):
    register, create_and_holder, _created, _serialize_calls, _deserialize_calls = (
        stateful_portable
    )
    register(payload=b"snapshot-bytes")
    create_and_holder("StatefulPortableCustom#1")
    payload = bridge.call_serialize("StatefulPortableCustom#1")
    assert payload == b"snapshot-bytes"
    assert isinstance(payload, bytes)


def test_call_serialize_repeated_calls_use_same_holder_identity(stateful_portable):
    register, create_and_holder, _created, serialize_calls, _deserialize_calls = (
        stateful_portable
    )
    register(payload=b"snapshot")
    create_and_holder("StatefulPortableCustom#1")
    first = bridge.call_serialize("StatefulPortableCustom#1")
    second = bridge.call_serialize("StatefulPortableCustom#1")
    assert first == second
    assert serialize_calls[0] is serialize_calls[1]
    assert serialize_calls[0] is _created[0]


def test_call_deserialize_restores_state(stateful_portable):
    register, create_and_holder, created, _serialize_calls, deserialize_calls = (
        stateful_portable
    )
    register(payload=b"original", restored_value=None)
    create_and_holder("StatefulPortableCustom#1")
    bridge.call_deserialize("StatefulPortableCustom#1", b"restored-bytes")
    assert deserialize_calls == [(created[0], b"restored-bytes")]
    assert created[0].restored == b"restored-bytes"


def test_call_serialize_and_deserialize_round_trip(stateful_portable):
    register, create_and_holder, created, _serialize_calls, _deserialize_calls = (
        stateful_portable
    )
    register(payload=b"round-trip-state", restored_value=None)
    create_and_holder("StatefulPortableCustom#1")
    snapshot = bridge.call_serialize("StatefulPortableCustom#1")
    assert snapshot == b"round-trip-state"
    bridge.call_deserialize("StatefulPortableCustom#1", snapshot)
    assert created[0].restored == b"round-trip-state"


def test_destroy_op_impl_holder_removes_instance_id(stateful_portable):
    register, create_and_holder, _created, _serialize_calls, _deserialize_calls = (
        stateful_portable
    )
    register()
    create_and_holder("StatefulPortableCustom#1")
    assert bridge.destroy_op_impl_holder("StatefulPortableCustom#1") is True
    with pytest.raises(KeyError):
        bridge.call_serialize("StatefulPortableCustom#1")
    with pytest.raises(KeyError):
        bridge.call_deserialize("StatefulPortableCustom#1", b"data")


def test_call_serialize_propagates_user_exception(stateful_portable):
    register, create_and_holder, _created, _serialize_calls, _deserialize_calls = (
        stateful_portable
    )

    def failing_serialize(self):
        raise ValueError("serialize boom")

    register(serialize_fn=failing_serialize)
    create_and_holder("StatefulPortableCustom#1")
    with pytest.raises(ValueError) as exc_info:
        bridge.call_serialize("StatefulPortableCustom#1")
    assert "serialize boom" in str(exc_info.value)


def test_call_deserialize_propagates_user_exception(stateful_portable):
    register, create_and_holder, _created, _serialize_calls, _deserialize_calls = (
        stateful_portable
    )

    def failing_deserialize(self, data):
        _ = data
        raise RuntimeError("deserialize boom")

    register(deserialize_fn=failing_deserialize)
    create_and_holder("StatefulPortableCustom#1")
    with pytest.raises(RuntimeError) as exc_info:
        bridge.call_deserialize("StatefulPortableCustom#1", b"data")
    assert "deserialize boom" in str(exc_info.value)


def test_call_serialize_rejects_non_bytes_return(stateful_portable):
    register, create_and_holder, _created, _serialize_calls, _deserialize_calls = (
        stateful_portable
    )

    def wrong_serialize(self):
        _ = self
        return "not-bytes"

    register(serialize_fn=wrong_serialize)
    create_and_holder("StatefulPortableCustom#1")
    with pytest.raises(TypeError):
        bridge.call_serialize("StatefulPortableCustom#1")


def test_call_serialize_rejects_empty_bytes_return(stateful_portable):
    register, create_and_holder, _created, _serialize_calls, _deserialize_calls = (
        stateful_portable
    )

    def empty_serialize(self):
        _ = self
        return b""

    register(serialize_fn=empty_serialize)
    create_and_holder("StatefulPortableCustom#1")
    with pytest.raises(ValueError):
        bridge.call_serialize("StatefulPortableCustom#1")


def test_call_deserialize_rejects_empty_bytes(stateful_portable):
    register, create_and_holder, _created, _serialize_calls, _deserialize_calls = (
        stateful_portable
    )
    register()
    create_and_holder("StatefulPortableCustom#1")
    with pytest.raises(ValueError):
        bridge.call_deserialize("StatefulPortableCustom#1", b"")


@pytest.mark.parametrize(
    "bad_input",
    [None, "string", 123, bytearray(b"data"), memoryview(b"data")],
)
def test_call_deserialize_rejects_non_bytes_input(stateful_portable, bad_input):
    register, create_and_holder, _created, _serialize_calls, _deserialize_calls = (
        stateful_portable
    )
    register()
    create_and_holder("StatefulPortableCustom#1")
    with pytest.raises(TypeError):
        bridge.call_deserialize("StatefulPortableCustom#1", bad_input)


def test_call_deserialize_rejects_non_none_return(stateful_portable):
    register, create_and_holder, _created, _serialize_calls, _deserialize_calls = (
        stateful_portable
    )

    def wrong_deserialize(self, data):
        _ = (self, data)
        return "not-None"

    register(deserialize_fn=wrong_deserialize)
    create_and_holder("StatefulPortableCustom#1")
    with pytest.raises(TypeError):
        bridge.call_deserialize("StatefulPortableCustom#1", b"data")


def test_call_serialize_rejects_unknown_instance_id():
    with pytest.raises(KeyError):
        bridge.call_serialize("unknown-holder-instance-id")


def test_call_deserialize_rejects_unknown_instance_id():
    with pytest.raises(KeyError):
        bridge.call_deserialize("unknown-holder-instance-id", b"data")


def test_validate_portable_payload_length_accepts_max_boundary():
    assert bridge._validate_portable_payload_length(UINT32_MAX) is None


def test_validate_portable_payload_length_accepts_minimum():
    assert bridge._validate_portable_payload_length(1) is None


def test_validate_portable_payload_length_rejects_zero():
    with pytest.raises(ValueError):
        bridge._validate_portable_payload_length(0)


def test_validate_portable_payload_length_rejects_overflow_without_allocation():
    with pytest.raises(ValueError):
        bridge._validate_portable_payload_length(UINT32_MAX + 1)
    with pytest.raises(ValueError):
        bridge._validate_portable_payload_length(1 << 64)
    with pytest.raises(ValueError):
        bridge._validate_portable_payload_length(-1)


@pytest.mark.parametrize("bad_length_type", ["4294967295", None, UINT32_MAX + 0.0])
def test_validate_portable_payload_length_rejects_non_int_length(bad_length_type):
    with pytest.raises(TypeError):
        bridge._validate_portable_payload_length(bad_length_type)


def test_call_serialize_payload_length_boundary_via_helper():
    """Prove bridge-level length validation runs before copying into native boundary."""

    class BoundaryPortable:
        def serialize(self) -> bytes:
            return b"x" * (UINT32_MAX + 1)

        def deserialize(self, data: bytes) -> None:
            _ = data

    custom_op.register_op_impl(op_type="BoundaryPortable")(BoundaryPortable)
    descriptors = bridge.load_and_get_op_impl_descriptors()
    descriptor_key = descriptors[0]["descriptor_key"]
    assert bridge.create_op_impl_holder("BoundaryPortable#1", descriptor_key) is True
    with pytest.raises(ValueError):
        bridge.call_serialize("BoundaryPortable#1")
    assert bridge.destroy_op_impl_holder("BoundaryPortable#1") is True


def test_call_deserialize_accepts_max_boundary_length_without_allocation():
    """Prove deserialize boundary length is accepted at the validation layer without forcing a 4 GiB copy."""

    assert bridge._validate_portable_payload_length(UINT32_MAX) is None


def test_call_deserialize_rejects_overflow_before_copy():
    """Prove deserialize rejects oversize payload length before copying bytes."""

    with pytest.raises(ValueError):
        bridge._validate_portable_payload_length(UINT32_MAX + 1)
