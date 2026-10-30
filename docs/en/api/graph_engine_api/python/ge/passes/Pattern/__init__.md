# \_\_init\_\_

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T03:57:40.705Z pushedAt=2026-10-16T10:53:07.631Z -->

## Applicable Products

All chips are supported.

## Description

Constructs a **Pattern** object from a pattern graph. After construction, the original **graph** handle is invalidated.

## Prototype

```python
__init__(graph: Graph) -> None
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| graph | Input | Pattern graph, of the type ge.graph.Graph. After construction, the graph handle is invalidated. |

## Return Value

None

## Constraints

After the construction is complete, do not continue to use the **Graph** variable.
