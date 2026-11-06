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

"""Registration-time checks for the Python custom-op Portable contract.

Only two things are enforced at registration time, both fail-fast with the
``op_type`` in the message: the serialize/deserialize pairing, and a zero-arg
constructible ``__init__`` (the framework builds the instance with
``descriptor.cls()``).  Only methods defined on the class itself are considered
(inherited attributes are ignored, so a plugin that happens to inherit an
unrelated ``serialize`` from a helper base class still registers).
Signature/annotation conformance is enforced at first use by
``_bridge.call_serialize``/``call_deserialize`` (bytes type, payload length,
``None`` return), which surface user-side ``TypeError`` naturally.
"""

import inspect


def _get_method(cls: type, method_name: str):
    """Fetch a method declared on the class itself; ignore inherited attributes."""
    return cls.__dict__.get(method_name)


def validate_portable_methods(cls: type, op_type: str) -> bool:
    """Return True when the class itself declares a paired serialize/deserialize."""
    serialize = _get_method(cls, "serialize")
    deserialize = _get_method(cls, "deserialize")
    if serialize is None and deserialize is None:
        return False
    if serialize is None or deserialize is None:
        present = "serialize" if serialize is not None else "deserialize"
        missing = "deserialize" if serialize is not None else "serialize"
        raise TypeError(
            f"invalid portable contract for op type {op_type}, method {missing}: "
            f"expected paired serialize/deserialize methods, actual {present} present"
        )
    # The framework builds the instance with descriptor.cls() (_bridge.py);
    # registration is the only point that can fail early with op_type in the message.
    try:
        inspect.signature(cls).bind()
    except (TypeError, ValueError) as exc:
        raise TypeError(
            f"invalid portable contract for op type {op_type}: __init__ must accept zero arguments: {exc}"
        ) from exc
    return True
