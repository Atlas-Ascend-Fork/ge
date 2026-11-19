# set\_data\_type

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T03:23:06.299Z pushedAt=2026-10-16T10:53:07.511Z -->

## Applicable Products

Supported on all chips.

## Description

Sets the data type of a tensor.

## Prototype

```python
set_data_type(data_type: DataType) -> Tensor
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| data_type | Input | Data type to be set. For details, see [DataType](../../DataType.md). |

## Return Value

\(Tensor\) Returns itself to support chained calls.

## Constraints

- If data\_type is not of the **DataType** type, a **TypeError** is thrown.
- If the setting fails, a **RuntimeError** is thrown.
