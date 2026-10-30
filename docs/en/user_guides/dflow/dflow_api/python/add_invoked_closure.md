# add\_invoked\_closure

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:02:13.872Z pushedAt=2026-10-29T02:54:28.727Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Adds a GraphProcessPoint or FlowGraphProcessPoint that invokes a FuncProcessPoint, and returns the added FuncProcessPoint.

## Prototype

```python
add_invoked_closure(graph_key, graph_pp)
add_invoked_closure(graph_key, flow_graph_pp)
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| graph_key | str | Unique identifier of the invoked GraphProcessPoint, which must be unique across the entire graph. |
| graph_pp | [GraphProcessPoint](dataflow-GraphProcessPoint.md) | Invoked GraphProcessPoint. For details, see [dataflow.GraphProcessPoint](dataflow-GraphProcessPoint.md). |
| flow_graph_pp | [FlowGraphProcessPoint](dataflow-FlowGraphProcessPoint.md) | Invoked FlowGraphProcessPoint. For details, see [dataflow.FlowGraphProcessPoint](dataflow-FlowGraphProcessPoint.md). |

## Return Value

Returns None in normal scenarios.

Returns "TypeError" if the parameter type is incorrect.

## Examples

```python
import dataflow as df
pp = df.FuncProcessPoint(...)
pp1 = df.GraphProcessPoint(...)
pp.add_invoked_closure(graph_key, pp1)
```

## Constraints

None
