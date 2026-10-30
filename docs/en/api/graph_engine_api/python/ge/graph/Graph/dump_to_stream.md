# dump\_to\_stream

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T03:00:37.677Z pushedAt=2026-10-16T10:53:07.417Z -->

## Applicable Products

Supported on all chips.

## Description

Exports the graph to a string stream.

## Prototype

```python
dump_to_stream(format: DumpFormat = DumpFormat.kReadable) -> str
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| **format** | Input | Export format, defaulting to **kReadable**. |

## Return Value

Returns the string representation of the graph, of type string.

## Constraints

- If **format** is not a valid **DumpFormat**, a **TypeError** is thrown.
- If the export fails, a **RuntimeError** is thrown.
