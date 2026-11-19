# set\_format

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T03:23:32.223Z pushedAt=2026-10-16T10:53:07.514Z -->

## Applicable Products

Supported on all chips.

## Description

Sets the tensor format.

## Prototype

```python
set_format(format: Format) -> Tensor
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| format | Input | Format to be set. For details about formats, see [Format](../../../ge/Format.md). |

## Return Value

\(Tensor\) Returns itself to support chained calls.

## Constraints

- If **format** is not of the **Format** type, raises **TypeError**.
- If the setting fails, raises **RuntimeError**.
