# -----------------------------------------------------------------------------------------------------------
# Copyright (c) 2026 Huawei Technologies Co., Ltd.
# This program is free software, you can redistribute it and/or modify it under the terms and conditions of
# CANN Open Software License Agreement Version 2.0 (the "License").
# Please refer to the License for details. You may not use this file except in compliance with the License.
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
# INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
# See LICENSE in the root of the software repository for the full text of the License.
# -----------------------------------------------------------------------------------------------------------

"""Shared model selection and deployment-specific request settings."""

import os

DEFAULT_MODEL = "glm-5.3-flash"
DEFAULT_CONCURRENCY = 64
DEFAULT_REPO_WORKERS = 8


class ModelMismatchError(ValueError):
    pass


def model_match_required(model):
    value = os.environ.get("LLM_REQUIRE_MODEL_MATCH", "").strip().lower()
    return (
        value in ("1", "true", "yes", "on") if value else model.lower() == DEFAULT_MODEL
    )


def resolve_model(explicit=None):
    return (explicit or os.environ.get("LLM_MODEL", "")).strip() or DEFAULT_MODEL


def add_model_argument(parser):
    parser.add_argument(
        "--model", help="Model override; default: LLM_MODEL or glm-5.3-flash"
    )


def configure_model(explicit=None):
    model = resolve_model(explicit)
    os.environ["LLM_MODEL"] = model
    return model


def request_settings(model, thinking, reasoning_effort):
    # This deployment always reasons and rejects disabled thinking with HTTP 400.
    if model.lower() == DEFAULT_MODEL and thinking == "disabled":
        return "enabled", "low"
    return thinking, reasoning_effort
