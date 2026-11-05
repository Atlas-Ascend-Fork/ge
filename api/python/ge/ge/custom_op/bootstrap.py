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

"""Discovery and loading utilities for Python GE custom op plugins."""

import logging
from types import ModuleType
from typing import List

from ge._internal.plugin_loader import load_plugins_from_env

from .registry import get_registered_op_impl_dicts

_LOGGER = logging.getLogger(__name__)

ENV_PY_CUSTOM_OP_PATH = "ASCEND_CUSTOM_OPP_PATH"


def load_custom_op_plugins() -> List[ModuleType]:
    """Load custom op plugins from the env-configured path list."""

    return load_plugins_from_env(
        ENV_PY_CUSTOM_OP_PATH,
        module_prefix="_ge_py_custom_op_",
        plugin_kind="python custom op",
    )


def get_registered_op_impls() -> List[dict]:
    return get_registered_op_impl_dicts()


def get_registered_op_protos() -> List[dict]:
    from .proto import get_registered_op_proto_dicts

    return get_registered_op_proto_dicts()


def freeze_deployment_artifacts() -> List[dict]:
    """Freeze and return the deployment artifacts of registered implementations.

    Called once right after the plugin modules are imported, so OM saving never
    re-reads the filesystem.  Every locatable definition file is frozen with all
    of its implementation classes; whether an implementation actually enters an
    OM is decided later by graph usage, not by a declaration on the class.
    Prototype declarations are collected as well: a prototype declared in a
    separate file gets its own artifact so the deployed environment can restore
    the ``register_op`` declaration (and with it the Python shape-inference
    capability).
    """

    from ._deployment import freeze_registered_artifacts
    from .proto import get_registered_op_protos as get_proto_descriptors
    from .registry import get_registered_op_impls as get_impl_descriptors

    artifacts = freeze_registered_artifacts(
        get_impl_descriptors(), get_proto_descriptors()
    )
    _warn_unpacked_protos(artifacts)
    return [item.to_bridge_dict() for item in artifacts]


def _warn_unpacked_protos(artifacts) -> None:
    """原型声明了 shape 推导、但其定义文件未随任何 artifact 打包时给出提示。"""

    packed_op_types = {
        descriptor.op_type
        for artifact in artifacts
        for descriptor in artifact.descriptors
    }
    for proto in get_registered_op_protos():
        op_type = proto.get("op_type")
        if op_type and op_type not in packed_op_types:
            _LOGGER.warning(
                "op type %s declares a Python prototype with a shape-inference "
                "function, but its definition file is not packed into the OM "
                "(its definition file could not be located, e.g. a dynamically "
                "created function). After "
                "loading this OM in another process, dynamic shape inference for "
                "this op is unavailable.",
                op_type,
            )
