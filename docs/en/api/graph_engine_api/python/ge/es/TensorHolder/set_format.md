# set\_format

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T02:56:53.050Z pushedAt=2026-10-16T10:53:07.403Z -->

## Applicable Products

Supported on all chips.

## Description

Sets the tensor data format.

## Prototype

```python
set_format(format: Format) -> TensorHolder
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| format | Input | Data format enumeration. See [Format](../../../../python/ge/Format.md). |

## Return Value

\(TensorHolder\) Returns itself to support chained calls.

## Constraints

- If **format** is not a **Format** enumeration, a **TypeError** is thrown.
- If the operation fails, a **RuntimeError** is thrown.
