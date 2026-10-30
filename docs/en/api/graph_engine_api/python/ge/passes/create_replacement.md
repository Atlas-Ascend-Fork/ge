# create\_replacement

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T03:46:43.590Z pushedAt=2026-10-16T10:53:07.585Z -->

## Applicable Products

Supported on all chips.

## Description

Creates a replacement graph to provide a replacement subgraph for pattern fusion or operator decomposition.

## Prototype

```python
create_replacement(graph: Graph) -> Graph
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| **graph** | Input | Replacement graph, of the type **ge.graph.Graph**. |

## Return Value

| Type | Description |
| --- | --- |
| **Graph** | Returns the passed-in replacement graph object. If the input is not of the **ge.graph.Graph** type, a **TypeError** is thrown. |

## Constraints

None
