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

"""Profiling-path contract tests for the GE MiniDAG adapter."""

import logging

import pytest

from python_minidag_adapter_test import DAGAdapter, FakeContext, _linear_graph
from ge.passes import _minidag_adapter as adapter_mod


@pytest.fixture
def adapter_caplog(caplog):
    """Capture adapter records straight off its own logger.

    Once the GE log bridge is installed the adapter logger stops propagating,
    so caplog's root handler no longer sees anything.  Attach that handler to
    the adapter logger directly to stay independent of install order.
    """

    logger = adapter_mod._LOGGER
    logger.addHandler(caplog.handler)
    try:
        yield caplog
    finally:
        logger.removeHandler(caplog.handler)


def test_unset_and_empty_path_disable_profiling(monkeypatch):
    monkeypatch.delenv("MINIDAG_PROFILING_PATH", raising=False)
    assert adapter_mod._resolve_profiling_path() == ""
    monkeypatch.setenv("MINIDAG_PROFILING_PATH", "")
    assert adapter_mod._resolve_profiling_path() == ""


@pytest.mark.parametrize(
    "value", ["/tmp/profiles", "/tmp/profiles.json", "/tmp/profiles.csv.bak"]
)
def test_non_csv_path_warns_and_is_ignored(monkeypatch, adapter_caplog, value):
    monkeypatch.setenv("MINIDAG_PROFILING_PATH", value)
    with adapter_caplog.at_level(logging.WARNING, logger=adapter_mod._LOGGER.name):
        assert adapter_mod._resolve_profiling_path() == ""
    assert "only accepts csv file path" in adapter_caplog.text


def test_csv_path_is_returned_verbatim_and_logs_info(
    monkeypatch, adapter_caplog, tmp_path
):
    csv_path = tmp_path / "op_summary.csv"
    csv_path.write_text("placeholder", encoding="utf-8")
    monkeypatch.setenv("MINIDAG_PROFILING_PATH", str(csv_path))
    with adapter_caplog.at_level(logging.INFO, logger=adapter_mod._LOGGER.name):
        assert adapter_mod._resolve_profiling_path() == str(csv_path)
    assert "MINIDAG_PROFILING_PATH" in adapter_caplog.text


def test_only_the_profiling_environment_variable_is_effective(monkeypatch, tmp_path):
    csv_path = tmp_path / "profile.csv"
    monkeypatch.setenv("MINIDAG_PROFILING_PATH", str(csv_path))
    monkeypatch.setenv("GE_PROFILING_PATH", "/other/profile.csv")
    monkeypatch.setenv("GE_PROFILING_DIR", "/other")
    assert adapter_mod._resolve_profiling_path() == str(csv_path)


def _write_profile(path, rows):
    path.write_text(
        "Op Name,Task Type,Task Duration(us),Block Num,Mix Block Num\n"
        + "\n".join(",".join(map(str, row)) for row in rows)
        + "\n",
        encoding="utf-8",
    )


def test_profile_hit_is_forwarded_to_plan_and_uses_weighted_strategy(
    monkeypatch, tmp_path
):
    import multistream

    path = tmp_path / "profile.csv"
    _write_profile(path, [("g_n0", "AI_CORE", "12.5", 7, 0)])
    monkeypatch.setenv("MINIDAG_PROFILING_PATH", str(path))
    calls = {}
    original_plan = multistream.plan

    def recording_plan(*args, **kwargs):
        calls.update(kwargs)
        return original_plan(*args, **kwargs)

    monkeypatch.setattr(multistream, "plan", recording_plan)
    graph = _linear_graph("g", 2)
    adapter = DAGAdapter()
    required = adapter.plan(
        graph,
        FakeContext(),
        max_stream_num=8,
        base_stream_id=0,
        merge_strategy="MainStream",
    )
    assert required >= 1
    assert calls["profiling_path"] == str(path)
    assert calls["merge_strategy"] is multistream.MergeStrategy.MAIN_STREAM
    # Strategy fallback occurs inside multistream.plan after a profile match;
    # the adapter still forwards the caller's requested strategy unchanged.
    assert adapter._nodes[0].get_node_cost().execution_time == -1.0


def test_invalid_profiling_file_fails_transaction(monkeypatch, tmp_path):
    path = tmp_path / "missing.csv"
    monkeypatch.setenv("MINIDAG_PROFILING_PATH", str(path))
    adapter = DAGAdapter()
    graph = _linear_graph()
    with pytest.raises(ValueError, match="cannot open profiling file"):
        adapter.plan(
            graph,
            FakeContext(),
            max_stream_num=8,
            base_stream_id=0,
            merge_strategy="LoadBalance",
        )
    assert adapter._plan is None
    assert adapter._graph is None
    assert adapter._nodes == []


def test_invalid_origin_marker_survives_and_profiling_path_is_forwarded(
    monkeypatch, tmp_path
):
    """The adapter must not overlay profiling data itself.

    A node whose GE stream id is INVALID is marked with ``execution_time ==
    0.0`` before the algorithm package ever sees it, and the adapter hands
    the CSV path over untouched.  Whether the package then overlays the
    profiled block counts on top of that marker is its own behaviour, covered
    by L1 (``graph-autofusion/multistream/tests/python/test_plan.py``);
    asserting it here would only be asserting against the stub.
    """

    import multistream

    path = tmp_path / "profile.csv"
    _write_profile(path, [("g_n0", "MIX_AIC", "12.5", 3, 5)])
    monkeypatch.setenv("MINIDAG_PROFILING_PATH", str(path))

    calls = {}
    original_plan = multistream.plan

    def recording_plan(*args, **kwargs):
        calls.update(kwargs)
        return original_plan(*args, **kwargs)

    monkeypatch.setattr(multistream, "plan", recording_plan)

    graph = _linear_graph("g", 1)
    graph._nodes[0].origin_stream_id = -1
    adapter = DAGAdapter()
    adapter.plan(
        graph,
        FakeContext(),
        max_stream_num=8,
        base_stream_id=0,
        merge_strategy="LoadBalance",
    )
    cost = adapter._nodes[0].get_node_cost()
    assert cost.execution_time == 0.0
    assert (cost.cube_block_num, cost.vec_block_num) == (0, 0)
    assert calls["profiling_path"] == str(path)
