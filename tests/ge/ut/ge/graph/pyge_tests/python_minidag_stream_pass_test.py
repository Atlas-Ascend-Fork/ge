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

"""End-to-end orchestration tests for ``DAGStreamPass.run``.

These tests stub only the adapter (the algorithm is covered by L1 and the
adapter itself by ``python_minidag_adapter_test.py``).  Keeping the context
call trace here makes the GE five-step reservation protocol and subgraph
isolation executable without a native GE session.
"""

import sys
from unittest import mock

import pytest


from _minidag_test_utils import import_stream_pass


stream_pass = import_stream_pass()
DAGStreamPass = stream_pass.DAGStreamPass


class TraceContext:
    def __init__(
        self, option=None, *, error_on=None, fail_reserve_at=None, setter_error=False
    ):
        self.option = option
        self.events = []
        self.error = None
        self.error_on = error_on
        self.fail_reserve_at = fail_reserve_at
        self.setter_error = setter_error
        self._next = 10
        self._reserve_count = 0

    def get_option_value(self, key):
        self.events.append(("option", key))
        return self.option

    def allocate_next_stream_id(self):
        value = self._next
        self._next += 1
        self._reserve_count += 1
        self.events.append(("reserve", value))
        if self.error_on == "reserve" or self._reserve_count == self.fail_reserve_at:
            raise RuntimeError("stream reservation failed")
        return value

    def set_error_message(self, message):
        self.events.append(("error", message))
        if self.setter_error:
            raise RuntimeError("error setter failed")
        self.error = message


class TraceGraph:
    def __init__(self, name="root", subgraphs=(), option_marker=...):
        self.name = name
        self._subgraphs = list(subgraphs)
        self.option_marker = option_marker

    def get_all_subgraphs(self):
        return list(self._subgraphs)

    def has_attr(self, key):
        return self.option_marker is not ...

    def get_attr(self, key):
        if self.option_marker is ...:
            raise RuntimeError("missing")
        return self.option_marker


class RecordingAdapter:
    instances = []
    required_by_graph = {}

    def __init__(self):
        self.calls = []
        RecordingAdapter.instances.append(self)

    def plan(self, graph, context, *, max_stream_num, base_stream_id, merge_strategy):
        context.events.append(("adapter_plan", graph.name))
        self.calls.append(
            ("plan", graph.name, max_stream_num, base_stream_id, merge_strategy)
        )
        return self.required_by_graph.get(graph.name, 2)

    def write_back(self, graph, context):
        context.events.append(("adapter_write_back", graph.name))
        self.calls.append(("write_back", graph.name))


@pytest.fixture(autouse=True)
def clear_adapter_instances():
    RecordingAdapter.instances.clear()
    RecordingAdapter.required_by_graph = {}
    yield
    RecordingAdapter.instances.clear()


def _patch_adapter():
    return mock.patch("ge.passes._minidag_adapter.DAGAdapter", RecordingAdapter)


def test_skip_and_unset_do_not_reserve_or_plan():
    for option, marker in [(None, ...), ("", ""), ("default", "default"), ("cv", "cv")]:
        graph = TraceGraph(option_marker=marker)
        context = TraceContext(option=option)
        with _patch_adapter():
            assert DAGStreamPass().run(graph, context) == 0
        assert not [event for event in context.events if event[0] == "reserve"]
        assert not RecordingAdapter.instances


@pytest.mark.parametrize("option", ["LoadBalance:8", "MainStream:4", "LoadBalance:1"])
def test_dag_path_uses_five_step_order(option):
    # Exercise the option fallback (the graph has no option attribute), so
    # the trace also proves resolution happens before reserving a stream.
    graph = TraceGraph(option_marker=...)
    context = TraceContext(option=option)
    with _patch_adapter():
        assert DAGStreamPass().run(graph, context) == 0
    adapter = RecordingAdapter.instances[0]
    assert [call[0] for call in adapter.calls] == ["plan", "write_back"]
    assert [event[0] for event in context.events] == [
        "option",
        "reserve",
        "adapter_plan",
        "reserve",
        "reserve",
        "adapter_write_back",
    ]
    plan_call = adapter.calls[0]
    expected_strategy = option.split(":", 1)[0]
    assert plan_call[2:] == (int(option.split(":", 1)[1]), 10, expected_strategy)
    # One base id plus two additional ids reserved by RecordingAdapter.
    assert [event[1] for event in context.events if event[0] == "reserve"] == [
        10,
        11,
        12,
    ]


def test_root_and_each_subgraph_get_independent_adapter_and_base_stream():
    sub_a = TraceGraph("sub_a", option_marker="LoadBalance:8")
    sub_b = TraceGraph("sub_b", option_marker="LoadBalance:8")
    root = TraceGraph("root", [sub_a, sub_b], option_marker="LoadBalance:8")
    RecordingAdapter.required_by_graph = {"root": 0, "sub_a": 1, "sub_b": 3}
    context = TraceContext()
    with _patch_adapter():
        assert DAGStreamPass().run(root, context) == 0
    adapters = RecordingAdapter.instances
    assert len(adapters) == 3
    assert [a.calls[0][1] for a in adapters] == ["root", "sub_a", "sub_b"]
    assert [a.calls[0][3] for a in adapters] == [10, 11, 13]
    # root reserves only base; subgraphs reserve base + required ids.
    assert [event[1] for event in context.events if event[0] == "reserve"] == [
        10,
        11,
        12,
        13,
        14,
        15,
        16,
    ]


def test_pass_instance_can_run_repeatedly_without_state_leak():
    pass_instance = DAGStreamPass()
    context_a = TraceContext(option="LoadBalance:8")
    context_b = TraceContext(option="LoadBalance:8")
    with _patch_adapter():
        assert pass_instance.run(TraceGraph("first"), context_a) == 0
        assert pass_instance.run(TraceGraph("second"), context_b) == 0
    assert [a.calls[0][1] for a in RecordingAdapter.instances] == ["first", "second"]
    assert [event[1] for event in context_a.events if event[0] == "reserve"] == [
        10,
        11,
        12,
    ]
    assert [event[1] for event in context_b.events if event[0] == "reserve"] == [
        10,
        11,
        12,
    ]


@pytest.mark.parametrize(
    "option, reason",
    [
        ("bad", "The format must be Strategy:N with one colon."),
        ("LoadBalance:0", "The stream number must be an integer in [1, 64]."),
        ("LoadBalance:65", "The stream number must be an integer in [1, 64]."),
        ("LoadBalance:8:2", "The format must be Strategy:N with one colon."),
    ],
)
def test_invalid_configuration_returns_false_and_sets_error(option, reason):
    graph = TraceGraph(option_marker=option)
    context = TraceContext(option="LoadBalance:8")
    with _patch_adapter():
        assert DAGStreamPass().run(graph, context) is False
    # Same fields as the C++ "%s is invalid, value=%s, reason=%s" error.
    assert (
        context.error
        == f"ge.autoMultistreamParallelMode is invalid, value={option}, reason={reason}"
    )
    assert not [event for event in context.events if event[0] == "reserve"]


def test_adapter_exception_is_intercepted_and_reported():
    graph = TraceGraph(option_marker="LoadBalance:8")
    context = TraceContext()

    class FailingAdapter(RecordingAdapter):
        def plan(self, *args, **kwargs):
            raise ValueError("allocation exploded")

    with mock.patch("ge.passes._minidag_adapter.DAGAdapter", FailingAdapter):
        assert DAGStreamPass().run(graph, context) is False
    assert (
        context.error
        == "root graph RunMiniDAGStreamPass failed, graph=root, reason=allocation exploded"
    )


def test_subgraph_failure_names_the_failing_subgraph():
    sub_a = TraceGraph("sub_a", option_marker="LoadBalance:8")
    sub_b = TraceGraph("sub_b", option_marker="LoadBalance:8")
    root = TraceGraph("root", [sub_a, sub_b], option_marker="LoadBalance:8")
    context = TraceContext()

    class FailOnSubB(RecordingAdapter):
        def plan(self, graph, *args, **kwargs):
            if graph.name == "sub_b":
                raise ValueError("allocation exploded")
            return super().plan(graph, *args, **kwargs)

    with mock.patch("ge.passes._minidag_adapter.DAGAdapter", FailOnSubB):
        assert DAGStreamPass().run(root, context) is False
    assert (
        context.error
        == "sub graph RunMiniDAGStreamPass failed, graph=sub_b, reason=allocation exploded"
    )


def test_base_stream_reservation_failure_skips_adapter():
    graph = TraceGraph(option_marker="LoadBalance:8")
    context = TraceContext(fail_reserve_at=1)
    with _patch_adapter():
        assert DAGStreamPass().run(graph, context) is False
    assert (
        context.error
        == "root graph RunMiniDAGStreamPass failed, graph=root, reason=stream reservation failed"
    )
    assert not RecordingAdapter.instances


def test_required_stream_reservation_failure_does_not_write_back():
    graph = TraceGraph(option_marker="LoadBalance:8")
    context = TraceContext(fail_reserve_at=2)
    with _patch_adapter():
        assert DAGStreamPass().run(graph, context) is False
    adapter = RecordingAdapter.instances[0]
    assert [call[0] for call in adapter.calls] == ["plan"]
    assert (
        context.error
        == "root graph RunMiniDAGStreamPass failed, graph=root, reason=stream reservation failed"
    )


def test_requested_mode_is_logged_like_cpp(caplog):
    graph = TraceGraph(option_marker=...)
    context = TraceContext(option="MainStream:4")
    logger = stream_pass._LOGGER
    # run() turns off propagation on this logger, so attach caplog directly.
    logger.addHandler(caplog.handler)
    caplog.set_level("INFO", logger=logger.name)
    try:
        with _patch_adapter():
            assert DAGStreamPass().run(graph, context) == 0
    finally:
        logger.removeHandler(caplog.handler)
    assert (
        "Auto multistream requested mode=MainStream:4, strategy=MainStream, max_stream_num=4."
        in caplog.text
    )


@pytest.fixture
def pass_errors(caplog):
    """Collect ERROR records straight off the pass logger.

    ``DAGStreamPass.run`` turns off propagation on this logger, so caplog's
    root handler alone may see nothing.
    """

    logger = stream_pass._LOGGER
    logger.addHandler(caplog.handler)
    caplog.set_level("INFO", logger=logger.name)
    try:
        yield lambda: [
            (record.filename, record.getMessage())
            for record in caplog.records
            if record.levelname == "ERROR"
        ]
    finally:
        logger.removeHandler(caplog.handler)


def test_invalid_configuration_is_logged_where_found(pass_errors):
    graph = TraceGraph(option_marker="LoadBalance:0")
    with _patch_adapter():
        assert DAGStreamPass().run(graph, TraceContext()) is False
    assert pass_errors() == [
        (
            "_minidag_stream_pass.py",
            "ge.autoMultistreamParallelMode is invalid, value=LoadBalance:0, "
            "reason=The stream number must be an integer in [1, 64].",
        )
    ]


def test_graph_failure_is_logged_where_found(pass_errors):
    sub = TraceGraph("sub", option_marker="LoadBalance:8")
    root = TraceGraph("root", [sub], option_marker="LoadBalance:8")

    class FailOnSub(RecordingAdapter):
        def plan(self, graph, *args, **kwargs):
            if graph.name == "sub":
                raise ValueError("allocation exploded")
            return super().plan(graph, *args, **kwargs)

    with mock.patch("ge.passes._minidag_adapter.DAGAdapter", FailOnSub):
        assert DAGStreamPass().run(root, TraceContext()) is False
    assert pass_errors() == [
        (
            "_minidag_stream_pass.py",
            "sub graph RunMiniDAGStreamPass failed, graph=sub, reason=allocation exploded",
        )
    ]


def test_run_routes_pass_records_to_ge_log():
    logger = stream_pass._LOGGER
    saved = (logger.level, logger.propagate, list(logger.handlers))
    try:
        with _patch_adapter():
            assert (
                DAGStreamPass().run(TraceGraph(option_marker="cv"), TraceContext()) == 0
            )
        from ge.passes import _minidag_adapter as adapter_mod

        assert [h for h in logger.handlers if isinstance(h, adapter_mod._GeLogHandler)]
        assert logger.propagate is False
    finally:
        for handler in list(logger.handlers):
            logger.removeHandler(handler)
        logger.setLevel(saved[0])
        logger.propagate = saved[1]
        for handler in saved[2]:
            logger.addHandler(handler)


def test_error_message_setter_failure_does_not_escape_run(monkeypatch):
    graph = TraceGraph(option_marker="invalid")
    context = TraceContext(setter_error=True)
    errors = []
    monkeypatch.setattr(
        stream_pass._LOGGER, "exception", lambda *args: errors.append(args)
    )
    with _patch_adapter():
        assert DAGStreamPass().run(graph, context) is False
    assert len(errors) == 1
    assert str(errors[0][1]) == "error setter failed"


def test_null_graph_or_context_is_rejected():
    with pytest.raises(RuntimeError, match="null Graph"):
        DAGStreamPass().run(None, TraceContext())
    with pytest.raises(RuntimeError, match="null StreamPassContext"):
        DAGStreamPass().run(TraceGraph(option_marker="cv"), None)


def test_builtin_descriptor_name_stage_and_kind_are_stable():
    descriptor = DAGStreamPass.__ge_pass_descriptor__
    assert descriptor.pass_name == "MiniDAGStreamPass"
    assert descriptor.kind == "stream"
    assert descriptor.stage == stream_pass.PassStage.AFTER_ASSIGN_LOGIC_STREAM


@pytest.mark.parametrize(
    "error", [KeyboardInterrupt(), SystemExit(7), RuntimeError("load failed")]
)
def test_builtin_load_preserves_exception_kind_and_diagnostics(monkeypatch, error):
    from ge.passes import bootstrap

    monkeypatch.setattr(bootstrap, "_builtin_load_error", None)
    monkeypatch.delitem(
        sys.modules, bootstrap.BUILTIN_STREAM_PASS_MODULE, raising=False
    )

    def fail_import(_name):
        raise error

    with mock.patch.object(
        bootstrap.importlib, "import_module", side_effect=fail_import
    ):
        with pytest.raises(type(error)) as caught:
            bootstrap.load_builtin_passes()
    assert bootstrap.get_builtin_load_error() is error
    if isinstance(error, Exception):
        assert caught.value.__cause__ is error
    else:
        assert caught.value is error
