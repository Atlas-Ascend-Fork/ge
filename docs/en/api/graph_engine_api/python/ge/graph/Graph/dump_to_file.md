# dump\_to\_file

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T03:00:16.625Z pushedAt=2026-10-16T10:53:07.416Z -->

## Applicable Products

Supported on all chips.

## Description

Exports the graph to a file.

## Prototype

```python
dump_to_file(format: DumpFormat = DumpFormat.kReadable, suffix: str = "") -> None
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| format | Input | Export format, defaulting to **kReadable**. For details, see [DumpFormat](../../../ge/DumpFormat.md). |
| suffix | Input | File name suffix, of the string type, defaulting to an empty string. |

## Return Value

None

## Constraints

- If **format** is not a valid **DumpFormat**, a **TypeError** is thrown.
- If **suffix** is not a string, a **TypeError** is thrown.
- If the export fails, a **RuntimeError** is thrown.
- The exported pbtxt format contains only the graph structure, without weight data or other attributes.
