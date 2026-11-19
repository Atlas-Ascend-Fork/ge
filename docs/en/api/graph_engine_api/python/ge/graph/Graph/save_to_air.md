# save\_to\_air

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T03:05:58.181Z pushedAt=2026-10-16T10:53:07.439Z -->

## Applicable Products

Supported on all chips.

## Description

Saves the graph in air format.

## Prototype

```python
save_to_air(file_path: str) -> None
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| **file_path** | Input | Path and file name for saving the air file, of the string type. |

## Return Value

None

## Constraints

- If **file\_path** is not a string, a TypeError is thrown.
- If saving fails, a RuntimeError is thrown.
