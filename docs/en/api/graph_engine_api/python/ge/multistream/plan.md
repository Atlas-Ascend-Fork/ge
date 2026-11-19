# plan

<!-- md-trans-meta sourceCommit=6e5f525dd9e972a6824fe2d1ae18cafa0d5e5675 translatedAt=2026-11-17T02:37:15.832Z pushedAt=2026-11-18T02:41:08.839Z nl=2 -->

## Prototype

```python
multistream.plan(
    graph: DAGGraph,
    *, max_stream_num: Optional[int] = None,
    base_stream_id: int = 0,
    merge_strategy: MergeStrategy = MergeStrategy.LOAD_BALANCE,
    profiling_path: Optional[str] = None,
) -> StreamPlan
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| graph | Input | [DAGGraph](DAGGraph.md) object. Mandatory. |
| max_stream_num | Input | Upper limit of the normal stream count, an integer from 1 to 64. `None` is treated as 8. |
| base_stream_id | Input | Starting point of stream IDs, a non-negative integer. Defaults to 0. |
| merge_strategy | Input | [MergeStrategy](MergeStrategy.md) enumeration. Defaults to load balancing. |
| profiling_path | Input | Path to the profiling CSV file. Supports a string or an `os.PathLike` object that returns a string path. Defaults to `None`; `None` or an empty string means that the profiling file is not read. |

## Return Value

Returns a [StreamPlan](StreamPlan.md). Throws an exception on failure and does not return partial results.

## Constraints

- All parameters except `graph` are keyword parameters; the stream count and the starting point do not accept bool.
- Planning does not modify the input graph or node costs, nor does it create Runtime streams.
- Prerequisites: prepare CANN Runtime and complete runtime initialization or offline target SoC configuration. If Runtime loading or core count query fails, report the exception to the caller instead of using the default core count.
- A graph type error or an integer parameter type error throws `TypeError`; an out-of-range stream count, a negative starting point, or an invalid policy throws `ValueError`; a cyclic graph or inconsistent internal results throws `RuntimeError`.
- Using the enumeration is recommended; the strings `LoadBalance`, `load_balance`, `MainStream`, and `main_stream` are compatible, case-insensitive, and support converting spaces and hyphens to underscores.
- When `profiling_path` is provided, the profiling CSV is read and nodes are matched by operator name. When valid records hit nodes that participate in normal path merging, the weighted load balancing policy is automatically adopted, overriding the selection of `merge_strategy`; otherwise, the specified policy is still used. If the file is unreadable or lacks required columns, `ValueError` is thrown.

## Examples

Prerequisites: prepare CANN Runtime and complete runtime initialization or offline target SoC configuration. For details, see [plan's Constraints](plan.md#constraints).

```python
import multistream as ms

graph = ms.DAGGraph("example")
source = graph.add_node("source", "Relu")
target = graph.add_node("target", "Relu")
graph.add_edge(source, 0, target, 0)
result = ms.plan(graph, max_stream_num=2, merge_strategy=ms.MergeStrategy.LOAD_BALANCE)
print(result.get_required_stream_count())
print(result.get_stream_id(source), result.get_stream_id(target))
```
