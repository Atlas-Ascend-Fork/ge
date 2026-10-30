# create\_pattern

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T03:40:55.660Z pushedAt=2026-10-16T10:53:07.582Z -->

## Applicable Products

Supported on all chips.

## Description

Wraps the input pattern graph into a native **Pattern** object for matching subgraphs in the target graph.

## Prototype

```python
create_pattern(graph: Graph) -> Pattern
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| graph | Input | Pattern graph, of the type **ge.graph.Graph**. |

## Return Value

| Type | Description |
| --- | --- |
| Pattern | Returns the constructed native **Pattern** object. |

## Constraints

None
