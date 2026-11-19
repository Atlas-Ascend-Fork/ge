# remove\_subgraph

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T03:05:25.701Z pushedAt=2026-10-16T10:53:07.437Z -->

## Applicable Products

Supported on all chips.

## Description

Removes the subgraph by name.

## Prototype

```python
remove_subgraph(name: str) -> None
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| name | Input | Subgraph name, of the string type. |

## Return Value

None

## Constraints

- If **name** is not a string, a TypeError is thrown.
- If the removal fails, a RuntimeError is thrown.
