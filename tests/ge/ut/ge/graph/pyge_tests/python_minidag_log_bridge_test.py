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

"""Logging bridge behaviour for embedded GE MiniDAG execution."""

import contextlib
import logging
import sys
import types

import pytest

from _minidag_test_utils import install_fake_multistream
from ge.passes import _minidag_adapter as adapter_mod


@contextlib.contextmanager
def _restored_logger(name):
    """Hand out a pristine logger and put the process-wide state back after."""

    logger = logging.getLogger(name)
    saved = (logger.level, logger.propagate, list(logger.handlers))
    for handler in list(logger.handlers):
        logger.removeHandler(handler)
    logger.setLevel(logging.NOTSET)
    logger.propagate = True
    try:
        yield logger
    finally:
        for handler in list(logger.handlers):
            logger.removeHandler(handler)
        logger.setLevel(saved[0])
        logger.propagate = saved[1]
        for handler in saved[2]:
            logger.addHandler(handler)


@pytest.fixture
def clean_algorithm_logger():
    with _restored_logger("multistream") as logger:
        yield logger


def test_load_installs_bridge_on_algorithm_and_adapter_loggers():
    # "multistream" carries the algorithm package's records and the adapter
    # logs under its own module name.  Neither is an ancestor of the other, so
    # a bridge on one alone would silently drop the other's diagnostics.
    install_fake_multistream()
    with _restored_logger("multistream") as algorithm_logger:
        with _restored_logger(adapter_mod._LOGGER.name) as adapter_logger:
            adapter_mod._load_multistream()
            for logger in (algorithm_logger, adapter_logger):
                handlers = [
                    h
                    for h in logger.handlers
                    if isinstance(h, adapter_mod._GeLogHandler)
                ]
                assert len(handlers) == 1
                assert logger.propagate is False


def test_install_is_idempotent_and_disables_propagation(clean_algorithm_logger):
    logger = clean_algorithm_logger
    adapter_mod._install_ge_log_handler()
    adapter_mod._install_ge_log_handler()
    handlers = [h for h in logger.handlers if isinstance(h, adapter_mod._GeLogHandler)]
    assert len(handlers) == 1
    assert logger.level == logging.DEBUG
    assert logger.propagate is False


def test_explicit_host_level_is_preserved(clean_algorithm_logger):
    logger = clean_algorithm_logger
    logger.setLevel(logging.ERROR)
    adapter_mod._install_ge_log_handler()
    assert logger.level == logging.ERROR
    assert logger.propagate is False


def test_emit_preserves_python_source_without_thread_prefix(
    monkeypatch, clean_algorithm_logger
):
    emitted = []
    native = types.ModuleType("ge.passes._native")
    native.GE_LOG_SOURCE_SUPPORTED = True
    native.emit_ge_log = lambda *args: emitted.append(args)
    monkeypatch.setitem(sys.modules, "ge.passes._native", native)
    handler = adapter_mod._GeLogHandler()
    record = logging.LogRecord(
        "multistream",
        logging.INFO,
        "/installed/multistream/algorithm/merger.py",
        277,
        "Use %s strategy.",
        ("kLoadBalance",),
        None,
    )
    record.thread = 12345
    record.funcName = "solve"
    handler.emit(record)
    assert emitted == [
        (logging.INFO, "Use kLoadBalance strategy.", "merger.py", 277, "solve")
    ]


def test_emit_preserves_traceback_and_stack_info(monkeypatch, clean_algorithm_logger):
    emitted = []
    native = types.ModuleType("ge.passes._native")
    native.GE_LOG_SOURCE_SUPPORTED = True
    native.emit_ge_log = lambda *args: emitted.append(args)
    monkeypatch.setitem(sys.modules, "ge.passes._native", native)
    try:
        raise ValueError("invalid profiling sample")
    except ValueError:
        record = logging.LogRecord(
            "multistream",
            logging.ERROR,
            "/installed/multistream/algorithm/planning.py",
            224,
            "stream allocation failed",
            (),
            sys.exc_info(),
            "plan",
            "stack info marker",
        )
    adapter_mod._GeLogHandler().emit(record)
    assert len(emitted) == 1
    level, message, filename, line, function = emitted[0]
    assert (level, filename, line, function) == (
        logging.ERROR,
        "planning.py",
        224,
        "plan",
    )
    assert message.startswith(
        "stream allocation failed\nTraceback (most recent call last):"
    )
    assert "ValueError: invalid profiling sample" in message
    assert message.endswith("stack info marker")


def test_emit_works_with_older_two_argument_native(monkeypatch, clean_algorithm_logger):
    emitted = []
    native = types.ModuleType("ge.passes._native")
    native.emit_ge_log = lambda level, message: emitted.append((level, message))
    monkeypatch.setitem(sys.modules, "ge.passes._native", native)
    record = logging.LogRecord(
        "multistream",
        logging.INFO,
        __file__,
        1,
        "progress %d%%",
        (100,),
        None,
        "solve",
    )
    adapter_mod._GeLogHandler().emit(record)
    assert emitted == [(logging.INFO, "solve:progress 100%")]


def test_emit_is_silent_when_native_bridge_is_unavailable(monkeypatch):
    monkeypatch.delitem(sys.modules, "ge.passes._native", raising=False)
    handler = adapter_mod._GeLogHandler()
    record = logging.LogRecord(
        "multistream", logging.INFO, __file__, 1, "message", (), None
    )
    # Import failure must not leak out of logging and turn a valid compile
    # into a pass failure.
    handler.emit(record)


def test_emit_swallows_native_bridge_errors(monkeypatch):
    native = types.ModuleType("ge.passes._native")

    def fail(*_args):
        raise RuntimeError("native logger unavailable")

    native.emit_ge_log = fail
    monkeypatch.setitem(sys.modules, "ge.passes._native", native)
    adapter_mod._GeLogHandler().emit(
        logging.LogRecord("multistream", logging.INFO, __file__, 1, "message", (), None)
    )
