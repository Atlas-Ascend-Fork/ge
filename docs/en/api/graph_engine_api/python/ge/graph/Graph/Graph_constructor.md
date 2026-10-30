# Graph Constructor

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T03:03:13.933Z pushedAt=2026-10-16T10:53:07.429Z -->

## Applicable Products

Supported on all chips.

## Description

Creates a new **Graph** object.

## Prototype

```python
Graph(name: Optional[str] = "graph")
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| name | Input | Graph name.<br>Type: string, defaulting to **graph**. |

## Return Value

Returns the created **Graph** object.

## Constraints

- If **name** is not of the string type, a **TypeError** is thrown.
- If graph creation fails, a **RuntimeError** is thrown.

## Examples

```python
graph = Graph("my_graph")
```
