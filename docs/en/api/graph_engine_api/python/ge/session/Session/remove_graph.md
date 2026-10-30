# remove\_graph

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T04:17:39.433Z pushedAt=2026-10-16T10:53:07.722Z -->

## Description

Removes a graph from the session.

## Prototype

```python
remove_graph(graph_id: int) -> None
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| graph_id | Input | Graph ID. |

## Return Value

None

## Constraints

- If **graph\_id** is not an integer, a **TypeError** is thrown.
- If the removal fails, a **RuntimeError** is thrown.
