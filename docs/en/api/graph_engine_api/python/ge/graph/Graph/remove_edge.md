# remove\_edge

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T03:04:46.417Z pushedAt=2026-10-16T10:53:07.434Z -->

## Applicable Products

Supported on all chips.

## Description

Removes an edge.

## Prototype

```python
remove_edge(src_node: Node, src_port_index: int, dst_node: Node, dst_port_index: int) -> None
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| **src_node** | Input | Source node of the edge. |
| **src_port_index** | Input | Source port index. Set to -1 when removing a control edge. |
| **dst_node** | Input | Target node of the edge. |
| **dst_port_index** | Input | Target port index. Set to -1 when removing a control edge. |

## Return Value

None

## Constraints

- If the parameter type is incorrect, a TypeError is thrown.
- If the removal operation fails, a RuntimeError is thrown.
