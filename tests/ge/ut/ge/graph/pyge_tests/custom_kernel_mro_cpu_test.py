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

import importlib.util
from pathlib import Path
import sys

import pytest


@pytest.fixture
def registry(monkeypatch):
    # This module is pure Python; no native classes or decorator internals are replaced.
    path = (
        Path(__file__).resolve().parents[6] / "api/python/ge/ge/custom_op/registry.py"
    )
    spec = importlib.util.spec_from_file_location("custom_registry_mro_cpu", path)
    api = importlib.util.module_from_spec(spec)
    monkeypatch.setitem(sys.modules, spec.name, api)
    spec.loader.exec_module(api)
    return api


def host_base(api):
    @api.register_op_impl(op_type="HostBase")
    class HostBase:
        @api.register_kernel(backend=api.OpBackend.HOST)
        def execute(self) -> None:
            self.called = "host"

    return HostBase


def test_compile_only_first_base_does_not_hide_visible_host_kernel(registry):
    api = registry

    @api.register_op_impl(op_type="CompileOnly")
    class CompileOnly:
        def compile(self) -> None:
            pass

    HostBase = host_base(api)

    @api.register_op_impl(op_type="Child")
    class Child(CompileOnly, HostBase):
        pass

    descriptor = Child.__ge_op_impl_descriptor__
    assert descriptor.interfaces == [
        api.INTERFACE_HOST_CPU_EXECUTE,
        api.INTERFACE_COMPILABLE,
    ]
    assert list(descriptor.kernel_bindings) == [api.OpBackend.HOST]
    child = Child()
    descriptor.kernel_bindings[api.OpBackend.HOST].function.__get__(child, Child)()
    assert child.called == "host"


def test_non_callable_intermediate_execute_shadows_base_binding(registry):
    api = registry

    class Shadow(host_base(api)):
        execute = None

        def compile(self) -> None:
            pass

    @api.register_op_impl(op_type="ShadowChild")
    class Child(Shadow):
        pass

    descriptor = Child.__ge_op_impl_descriptor__
    assert not descriptor.kernel_bindings
    assert descriptor.interfaces == [api.INTERFACE_COMPILABLE]
    assert Child().execute is None


def test_ordinary_registered_host_inheritance_is_retained(registry):
    api = registry

    @api.register_op_impl(op_type="SimpleChild")
    class Child(host_base(api)):
        pass

    assert list(Child.__ge_op_impl_descriptor__.kernel_bindings) == [api.OpBackend.HOST]


def test_bare_execute_inheritance_keeps_device_default(registry):
    api = registry

    class Base:
        def execute(self) -> None:
            pass

    @api.register_op_impl(op_type="BareChild")
    class Child(Base):
        pass

    assert list(Child.__ge_op_impl_descriptor__.kernel_bindings) == [
        api.OpBackend.DEVICE
    ]


def test_local_backend_declaration_keeps_precedence(registry):
    api = registry

    @api.register_op_impl(op_type="LocalChild")
    class Child(host_base(api)):
        @api.register_kernel(backend=api.OpBackend.DEVICE)
        def execute(self) -> None:
            pass

    assert list(Child.__ge_op_impl_descriptor__.kernel_bindings) == [
        api.OpBackend.DEVICE
    ]
