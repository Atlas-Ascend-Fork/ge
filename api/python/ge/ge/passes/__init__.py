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

"""Python GE passes public package."""

__all__ = [
    "DecomposePass",
    "FuseCheckResult",
    "FusionBasePass",
    "MatchResult",
    "NodeIo",
    "PassStage",
    "PassContext",
    "Pattern",
    "PatternMatcherConfig",
    "PatternMatcherConfigBuilder",
    "PatternFusionPass",
    "ResourcePlanPass",
    "StreamPass",
    "StreamPassContext",
    "SubgraphInput",
    "SubgraphOutput",
    "SubgraphBoundary",
    "SubgraphRewriter",
    "can_fuse",
    "clear_registered_passes",
    "create_pattern",
    "create_replacement",
    "get_registered_pass_by_descriptor_key",
    "get_registered_pass_dicts",
    "get_registered_passes",
    "infer_shape",
    "pattern",
    "register_decompose_pass",
    "register_fusion_pass",
    "register_resource_plan_pass",
    "report_fuse",
    "report_match",
]

import sys as _sys
from types import ModuleType as _ModuleType


_LAZY_EXPORTS = {
    "DecomposePass": ".base",
    "FuseCheckResult": ".fuse_inspector",
    "FusionBasePass": ".base",
    "MatchResult": ".base",
    "PassStage": ".base",
    "PassContext": ".base",
    "PatternMatcherConfig": ".base",
    "PatternMatcherConfigBuilder": ".base",
    "PatternFusionPass": ".base",
    "ResourcePlanPass": ".base",
    "StreamPass": ".base",
    "StreamPassContext": ".base",
    "SubgraphInput": ".base",
    "SubgraphOutput": ".base",
    "SubgraphBoundary": ".base",
    "SubgraphRewriter": ".base",
    "infer_shape": ".base",
    "can_fuse": ".fuse_inspector",
    "NodeIo": ".pattern",
    "Pattern": ".pattern",
    "create_pattern": ".pattern",
    "create_replacement": ".replacement",
    "clear_registered_passes": ".registry",
    "get_registered_pass_by_descriptor_key": ".registry",
    "get_registered_pass_dicts": ".registry",
    "get_registered_passes": ".registry",
    "register_decompose_pass": ".registry",
    "register_fusion_pass": ".registry",
    "register_resource_plan_pass": ".registry",
    "report_fuse": ".fuse_inspector",
    "report_match": ".fuse_inspector",
    # Keep the pattern decorator lazy as well.  Importing ``ge.passes`` for a
    # stream-pass bootstrap must not force the graph C wrapper/native artifact
    # to load before a pass is actually used.  Attribute lookups are served by
    # the ``_PassesModule.pattern`` descriptor below, which shadows the
    # same-named submodule; this entry stays for the explicit lazy path.
    "pattern": ".pattern",
}


def __getattr__(name: str):
    module_name = _LAZY_EXPORTS.get(name)
    if module_name is None:
        raise AttributeError(f"module {__name__!r} has no attribute {name!r}")

    import importlib

    module = importlib.import_module(module_name, __name__)
    value = getattr(module, name)
    globals()[name] = value
    return value


def __dir__():
    return sorted(set(globals()) | set(__all__))


class _PassesModule(_ModuleType):
    """Module type keeping ``ge.passes.pattern`` resolved to the decorator.

    This package exports the ``pattern`` decorator but also owns a ``pattern``
    submodule.  Importing that submodule (``import ge.passes.pattern`` from the
    native bindings, or ``from .pattern import ...`` inside this package) makes
    the import machinery bind the submodule as a package attribute, which would
    shadow the decorator and stop the lazy ``__getattr__`` above from running.
    A property is a data descriptor on the module type, so it wins over that
    binding for every import order, while ``sys.modules["ge.passes.pattern"]``
    keeps serving the submodule itself.
    """

    @property
    def pattern(self):
        # Resolved on demand, so importing ``ge.passes`` stays lazy.
        from .pattern import pattern

        return pattern

    @pattern.setter
    def pattern(self, value):
        # Drop the submodule binding written by the import machinery; keeping
        # it would be a no-op anyway because this descriptor takes precedence.
        pass


_sys.modules[__name__].__class__ = _PassesModule
