# add\_data\_edge

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T02:59:29.721Z pushedAt=2026-10-16T10:53:07.413Z -->

## Applicable Products

Supported on all chips.

## Description

Adds a data edge.

## Prototype

```python
add_data_edge(src_node: Node, src_port_index: int, dst_node: Node, dst_port_index: int) -> None
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| **src_node** | Input | Source node of the edge. |
| **src_port_index** | Input | Source port index. |
| **dst_node** | Input | Destination node of the edge. |
| **dst_port_index** | Input | Destination port index. |

## Return Value

None

## Constraints

- If the parameter type is incorrect, a TypeError is thrown.
- If the add operation fails, a RuntimeError is thrown.
