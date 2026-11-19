# add\_subgraph

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T02:59:47.421Z pushedAt=2026-10-16T10:53:07.415Z -->

## Applicable Products

Supported on all chips.

## Description

Adds a subgraph to the graph.

## Prototype

```python
add_subgraph(subgraph: Graph) -> None
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| **subgraph** | Input | Subgraph object to be added. |

## Return Value

None

## Constraints

- If **subgraph** is not of the **Graph** type, a **TypeError** is thrown.
- Subgraphs are indexed by name, and subgraph names within a parent graph must be unique.
- If the addition fails, a **RuntimeError** is thrown.
