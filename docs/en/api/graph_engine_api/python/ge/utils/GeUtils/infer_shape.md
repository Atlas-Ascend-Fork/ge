# infer\_shape

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T04:20:30.322Z pushedAt=2026-10-16T10:53:07.733Z -->

## Applicable Products

Supported on all chips.

## Description

Given the input shape, performs shape inference on the entire **Graph**.

This API only performs shape inference and does not apply any other optimization to the graph (such as constant folding or dead edge elimination).

## Prototype

```python
infer_shape(graph: Graph, input_shapes: List[List[int]]) -> None
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| **graph** | Input | Graph to be inferred. |
| **input_shapes** | Input | Shape corresponding to the graph input. |

## Return Value

None

## Constraints

- If input\_shapes is not a list of integer lists, a **TypeError** is thrown.
- If shape inference fails, a **RuntimeError** is thrown.
