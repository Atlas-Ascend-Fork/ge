# run\_graph

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T04:18:35.889Z pushedAt=2026-10-16T10:53:07.723Z -->

## Description

Runs the graph.

## Prototype

```python
run_graph(graph_id: int, inputs: List[Tensor]) -> List[Tensor]
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| **graph_id** | Input | Graph ID. |
| **inputs** | Input | Input tensor list. |

## Return Value

\(List\[Tensor\]\) Returns the list of output tensors.

## Constraints

- If **graph\_id** is not an integer, a **TypeError** is thrown.
- If any element in **inputs** is not of the **Tensor** type, a **TypeError** is thrown.
- If the execution fails, a **RuntimeError** is thrown.
- For more constraints, see [Constraints](../../../../cpp/ge/Session/RunGraph.md#constraints).
