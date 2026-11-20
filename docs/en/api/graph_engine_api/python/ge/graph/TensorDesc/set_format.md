# set\_format

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T03:28:09.207Z pushedAt=2026-10-16T10:53:07.531Z -->

## Applicable Products

Supported on all chips.

## Description

Sets the **Format** of a **Tensor** in **TensorDesc**.

## Prototype

```python
set_format(format: Format) -> TensorDesc
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| format | Input | New data format, of the **Format** type. |

## Return Value

\(TensorDesc\) Returns itself to support chained calls.

## Constraints

If **format** is not of the **Format** type, a **TypeError** is thrown.
