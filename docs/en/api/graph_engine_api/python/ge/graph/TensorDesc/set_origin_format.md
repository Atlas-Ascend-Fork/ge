# set\_origin\_format

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T03:28:11.815Z pushedAt=2026-10-16T10:53:07.532Z -->

## Applicable Products

Supported on all chips.

## Description

Sets the original **Format** of the tensor in **TensorDesc**. This **Format** refers to the format of the original network model.

## Prototype

```python
set_origin_format(format: Format) -> TensorDesc
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| **format** | Input | Original data format, of the **Format** type. |

## Return Value

\(TensorDesc\) Returns itself to support chained calls.

## Constraints

- If **format** is not of the **Format** type, a **TypeError** is thrown.
