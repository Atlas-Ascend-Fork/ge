# set\_data\_type

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T02:56:39.086Z pushedAt=2026-10-16T10:53:07.401Z -->

## Applicable Products

Supported on all chips.

## Description

Sets the tensor data type.

## Prototype

```python
set_data_type(data_type: DataType) -> TensorHolder
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| **data_type** | Input | Data type enum. For details, see [DataType](../../DataType.md). |

## Return Value

\(TensorHolder\) Returns itself to support chained calls.

## Constraints

- If data\_type is not a **DataType** enum, a **TypeError** is thrown.
- If the operation fails, a **RuntimeError** is thrown.
