# add\_control\_edge

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T02:59:08.594Z pushedAt=2026-10-16T10:53:07.410Z -->

## Applicable Products

Supported on all chips.

## Description

Adds a control edge.

## Prototype

```python
add_control_edge(src_node: Node, dst_node: Node) -> None
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| src_node | Input | Source node of the edge. |
| dst_node | Input | Destination node of the edge. |

## Return Value

None

## Constraints

- If the parameter type is incorrect, a **TypeError** is thrown.
- If the add operation fails, a **RuntimeError** is thrown.
