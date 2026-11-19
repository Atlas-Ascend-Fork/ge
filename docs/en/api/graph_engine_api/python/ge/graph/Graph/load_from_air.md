# load\_from\_air

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T03:03:56.436Z pushedAt=2026-10-16T10:53:07.430Z -->

## Applicable Products

Supported on all chips.

## Description

Loads a graph from the air format.

## Prototype

```python
load_from_air(file_path: str) -> None
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| **file_path** | Input | Path and file name of the air file to load, of string type. |

## Return Value

None

## Constraints

- If **file\_path** is not a string, a TypeError is raised.
- If loading fails, a RuntimeError is raised.
