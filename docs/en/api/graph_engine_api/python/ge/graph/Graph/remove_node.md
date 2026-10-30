# remove\_node

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T03:05:11.685Z pushedAt=2026-10-16T10:53:07.435Z -->

## Applicable Products

Supported on all chips.

## Description

Removes a node from the graph.

## Prototype

```python
remove_node(node: Node) -> None
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| **node** | Input | Node to be removed. |

## Return Value

None

## Constraints

- If **node** is not of the **Node** type, a **TypeError** is thrown.
- If the removal operation fails, a **RuntimeError** is thrown.
