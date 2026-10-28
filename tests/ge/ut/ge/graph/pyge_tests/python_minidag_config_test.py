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

"""Configuration grammar and precedence gates for the Python MiniDAG pass.

The C++ stream pass accepts only the exact ``Strategy:positive_integer``
grammar.  These tests keep the Python parser's four-state result (unset,
skip, dag, error) explicit so malformed options cannot silently select a
different algorithm or fall back to a context option.
"""

import pytest

from _minidag_test_utils import import_stream_pass


stream_pass = import_stream_pass()
_parse = stream_pass._parse_auto_multistream
_resolve = stream_pass._resolve_auto_multistream_config

# Reasons printed by StreamUtils::ParseAutoMultistreamParallelMode.
_FORMAT = "The format must be Strategy:N with one colon."
_STRATEGY = "The strategy must be LoadBalance or MainStream."
_STREAM_NUM = "The stream number must be an integer in [1, 64]."
_NOT_STRING = "The value must be a string."


class GraphWithAttr:
    def __init__(self, value, *, has_attr=True, get_error=None):
        self.value = value
        self._has_attr = has_attr
        self._get_error = get_error

    def has_attr(self, _key):
        return self._has_attr

    def get_attr(self, _key):
        if self._get_error is not None:
            raise self._get_error
        return self.value


class OptionContext:
    def __init__(self, value=None, error=None):
        self.value = value
        self.error = error
        self.calls = 0

    def get_option_value(self, _key):
        self.calls += 1
        if self.error is not None:
            raise self.error
        return self.value


@pytest.fixture
def pass_caplog(caplog):
    """Capture records straight off the pass logger.

    ``DAGStreamPass.run`` installs the GE bridge on this logger and turns off
    propagation, so caplog's root handler alone may see nothing.
    """

    logger = stream_pass._LOGGER
    logger.addHandler(caplog.handler)
    caplog.set_level("INFO", logger=logger.name)
    try:
        yield caplog
    finally:
        logger.removeHandler(caplog.handler)


@pytest.mark.parametrize(
    "value, from_graph, state, strategy, maximum",
    [
        (None, False, "unset", None, None),
        ("", False, "unset", None, None),
        ("default", True, "skip", None, None),
        ("default", False, "error", None, None),
        ("cv", False, "skip", None, None),
        ("LoadBalance:1", False, "dag", "LoadBalance", 1),
        ("MainStream:4", False, "dag", "MainStream", 4),
        ("LoadBalance:64", False, "dag", "LoadBalance", 64),
        ("LoadBalance:8", True, "dag", "LoadBalance", 8),
    ],
)
def test_parser_four_state_matrix(value, from_graph, state, strategy, maximum):
    result = _parse(value, from_graph=from_graph)
    assert result.state == state
    assert result.strategy == strategy
    assert result.max_stream_num == maximum


def test_dag_config_keeps_the_configured_text():
    assert _parse("MainStream:4", from_graph=False).value == "MainStream:4"


@pytest.mark.parametrize(
    "value, reason",
    [
        ("LoadBalance", _FORMAT),
        ("随便一串", _FORMAT),
        ("LoadBalance:0", _STREAM_NUM),
        ("LoadBalance:65", _STREAM_NUM),
        ("LoadBalance:-1", _STREAM_NUM),
        ("LoadBalance:99999999999999", _STREAM_NUM),
        ("LoadBalance:0x8", _STREAM_NUM),
        ("LoadBalance:8 ", _STREAM_NUM),
        ("LoadBalance:+8", _STREAM_NUM),
        ("LoadBalance:8:2", _FORMAT),
        (":8", _STRATEGY),
        ("Unknown:8", _STRATEGY),
        ("LoadBalance:", _STREAM_NUM),
        # WeightedLoadBalance 已删除：带流数也不再是合法策略。
        ("WeightedLoadBalance:8", _STRATEGY),
        ("WeightedLoadBalance", _FORMAT),
        (8, _NOT_STRING),
        (b"LoadBalance:8", _NOT_STRING),
    ],
)
def test_parser_rejects_noncanonical_or_invalid_values(value, reason):
    result = _parse(value, from_graph=False)
    assert result.state == "error"
    assert result.reason == reason
    assert result.value == (value if isinstance(value, str) else repr(value))


def test_default_from_the_option_reports_the_cpp_reason():
    result = _parse("default", from_graph=False)
    assert (result.state, result.value, result.reason) == (
        "error",
        "default",
        "The default mode is supported only by the graph attribute set by a custom pass.",
    )


def test_graph_attribute_has_priority_over_context_option():
    graph = GraphWithAttr("MainStream:4")
    context = OptionContext("LoadBalance:8")
    result = _resolve(graph, context)
    assert (result.state, result.strategy, result.max_stream_num) == (
        "dag",
        "MainStream",
        4,
    )
    assert context.calls == 0


def test_graph_attribute_source_is_logged_like_cpp(pass_caplog):
    _resolve(GraphWithAttr("MainStream:4"), OptionContext("LoadBalance:8"))
    assert (
        "Use auto multistream parallel mode MainStream:4 from graph attribute."
        in pass_caplog.text
    )


def test_option_source_is_not_logged_as_graph_attribute(pass_caplog):
    _resolve(GraphWithAttr(None, has_attr=False), OptionContext("LoadBalance:8"))
    assert "from graph attribute" not in pass_caplog.text


def test_invalid_explicit_graph_attribute_does_not_fallback_to_option():
    graph = GraphWithAttr("not-a-mode")
    context = OptionContext("LoadBalance:8")
    result = _resolve(graph, context)
    assert result.state == "error"
    assert context.calls == 0


def test_graph_without_has_attr_uses_option_when_getter_reports_missing():
    class LegacyGraph:
        def get_attr(self, _key):
            raise RuntimeError("missing")

    context = OptionContext("LoadBalance:8")
    result = _resolve(LegacyGraph(), context)
    assert (result.state, result.strategy, result.max_stream_num) == (
        "dag",
        "LoadBalance",
        8,
    )
    assert context.calls == 1


def test_graph_without_has_attr_still_uses_successful_graph_value():
    class LegacyGraph:
        def get_attr(self, _key):
            return "MainStream:4"

    context = OptionContext("LoadBalance:8")
    result = _resolve(LegacyGraph(), context)
    assert (result.state, result.strategy, result.max_stream_num) == (
        "dag",
        "MainStream",
        4,
    )
    assert context.calls == 0


def test_graph_has_attr_true_but_getter_fails_is_an_error_not_fallback():
    graph = GraphWithAttr("ignored", get_error=RuntimeError("broken attr"))
    context = OptionContext("LoadBalance:8")
    result = _resolve(graph, context)
    assert result.state == "error"
    assert (result.value, result.reason) == ("", "The graph attribute cannot be read.")
    assert context.calls == 0


def test_option_getter_failure_means_unset():
    class NoOptionGraph:
        def has_attr(self, _key):
            return False

    context = OptionContext(error=RuntimeError("no option"))
    assert _resolve(NoOptionGraph(), context).state == "unset"


def test_resolution_has_no_context_side_effects():
    graph = GraphWithAttr("LoadBalance:8")
    context = OptionContext("MainStream:4")
    before = dict(context.__dict__)
    _resolve(graph, context)
    assert context.__dict__ == before
