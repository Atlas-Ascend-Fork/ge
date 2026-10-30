# get\_subgraph

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T03:02:59.705Z pushedAt=2026-10-16T10:53:07.426Z -->

## Applicable Products

Supported on all chips.

## Description

Obtains the subgraph by name.

## Prototype

```python
get_subgraph(name: str) -> Optional[Graph]
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| **name** | Input | Subgraph name, of the string type. |

## Return Value

\(Graph | None\) Subgraph object, or **None** if not found.

## Constraints

- If **name** is not a string, a TypeError is thrown.
- If the acquisition fails, a RuntimeError is thrown.
