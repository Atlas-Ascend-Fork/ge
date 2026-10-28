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

"""Discovery and loading utilities for Python GE pass plugins.

The built-in MiniDAG resource-plan pass is deliberately loaded separately from the
environment plugin path.  Loading its descriptor is cheap (the ``multistream``
algorithm package itself is imported lazily by the pass callback), and doing
it whenever the bridge is loaded supports both explicitly configured Python
plugins and on-demand registration before DAG stream allocation.
"""

import importlib
import sys
from types import ModuleType
from typing import List, Optional

from ge._internal.plugin_loader import load_plugins_from_env
from .registry import (
    PASS_KIND_STREAM,
    get_registered_pass_dicts,
    get_registered_passes as _get_registered_passes,
)

ENV_PY_PASS_PATH = "ASCEND_GE_PY_PASS_PATH"
BUILTIN_STREAM_PASS_MODULE = "ge.passes._minidag_stream_pass"

_builtin_load_error: Optional[BaseException] = None


def _builtin_descriptor_is_registered(module: ModuleType) -> bool:
    module_name = getattr(module, "__name__", BUILTIN_STREAM_PASS_MODULE)
    return any(
        descriptor.module_name == module_name
        and descriptor.kind == PASS_KIND_STREAM
        and descriptor.pass_name == "MiniDAGStreamPass"
        for descriptor in _get_registered_passes()
    )


def load_builtin_passes() -> List[ModuleType]:
    """Load the built-in stream pass descriptor and return its module.

    ``clear_registered_passes()`` intentionally does not unload Python
    modules.  If the registry was reset during a bridge unload/reload cycle,
    explicitly reload the already-imported module so its decorator runs again.
    Import failures are retained for diagnostics and re-raised; callers must
    not turn a missing built-in descriptor into a successful single-stream
    compilation.
    """

    global _builtin_load_error
    try:
        module = sys.modules.get(BUILTIN_STREAM_PASS_MODULE)
        if module is None:
            module = importlib.import_module(BUILTIN_STREAM_PASS_MODULE)
        elif not _builtin_descriptor_is_registered(module):
            module = importlib.reload(module)
        if not _builtin_descriptor_is_registered(module):
            raise RuntimeError(
                "built-in MiniDAG stream pass did not register a descriptor"
            )
    except Exception as exc:
        _builtin_load_error = exc
        raise RuntimeError(
            f"failed to load built-in MiniDAG stream pass descriptor: {exc}"
        ) from exc
    except BaseException as exc:
        _builtin_load_error = exc
        raise
    _builtin_load_error = None
    return [module]


def get_builtin_load_error() -> Optional[BaseException]:
    """Return the last built-in load exception, if any (bridge diagnostics)."""

    return _builtin_load_error


def load_pass_plugins() -> List[ModuleType]:
    """Load the built-in pass and env-configured third-party plugins."""

    modules = load_builtin_passes()
    seen = {getattr(module, "__name__", "") for module in modules}
    for module in load_plugins_from_env(
        ENV_PY_PASS_PATH, module_prefix="_ge_py_pass_", plugin_kind="python pass"
    ):
        module_name = getattr(module, "__name__", "")
        if module_name not in seen:
            modules.append(module)
            seen.add(module_name)
    return modules


def get_registered_passes() -> List[dict]:
    return get_registered_pass_dicts()
