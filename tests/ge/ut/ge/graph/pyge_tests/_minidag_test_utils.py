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

"""Small import helper for MiniDAG Python-pass unit tests.

The GE wheel's native pass artifact is intentionally optional when running
source-tree unit tests.  Configuration/orchestration tests exercise Python
logic only, so a tiny stand-in for the native symbols lets them run on a
normal developer machine while native CI still imports the real artifact.

The ``multistream`` package is handled differently again: it belongs to
graph-autofusion, so GE's unit tests substitute the stub in
``_fake_multistream`` rather than depending on another repository being
present.
"""

import importlib
import sys
import types


def import_stream_pass():
    try:
        return importlib.import_module("ge.passes._minidag_stream_pass")
    except ImportError:
        previous_native = sys.modules.get("ge.passes._native")
        native = types.ModuleType("ge.passes._native")

        class _NativeType:
            pass

        for name in (
            "MatchResult",
            "PassContext",
            "PatternMatcherConfig",
            "PatternMatcherConfigBuilder",
            "SubgraphBoundary",
            "SubgraphInput",
            "SubgraphOutput",
            "SubgraphRewriter",
            "StreamPassContext",
        ):
            setattr(native, name, type(name, (_NativeType,), {}))
        native.infer_shape = lambda *_args, **_kwargs: None
        sys.modules["ge.passes._native"] = native
        try:
            return importlib.import_module("ge.passes._minidag_stream_pass")
        finally:
            # Do not make a fake native module visible to unrelated tests; the
            # imported base/registry classes retain their references locally.
            if previous_native is None:
                sys.modules.pop("ge.passes._native", None)
            else:
                sys.modules["ge.passes._native"] = previous_native


def install_fake_multistream():
    """Install the in-repo stub as ``multistream`` for this test session.

    ``multistream`` ships from graph-autofusion, so a GE unit test must not
    require that repository to be present.  The stub is installed
    unconditionally -- also when a real package happens to be importable --
    so the GE gate stays hermetic and gives the same result on every machine.
    Integration against the real package is L4's job, where "multistream 可导入"
    is a declared precondition.
    """

    import _fake_multistream  # noqa: PLC0415

    module = _fake_multistream.build_module()
    sys.modules["multistream"] = module
    return module
