# build\_and\_reset

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T02:42:49.086Z pushedAt=2026-10-16T10:53:07.354Z -->

## Applicable Products

Supported on all chips.

## Description

Builds a graph.

## Prototype

```python
build_and_reset(outputs: Optional[List[TensorHolder]] = None) -> Graph
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| outputs | Input | Optional parameter. List of **TensorHolder** objects to be set as graph outputs. If provided, all outputs are automatically set before the graph is built. Output indices are assigned sequentially starting from 0. If **None**, the graph is built using the previously set outputs. |

## Return Value

\(Graph\) Returns the constructed graph object.

## Constraints

- After calling **build\_and\_reset\(\)**, the builder enters the built state and cannot be used to create new tensors.
- A new **GraphBuilder** is required to build another graph.
- If **outputs** is not a list of **TensorHolder** objects, a **TypeError** is raised.
- If the build fails, a **RuntimeError** is raised.
