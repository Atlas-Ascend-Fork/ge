# set\_data\_type

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T03:27:28.837Z pushedAt=2026-10-16T10:53:07.529Z -->

## Applicable Products

Supported on all chips.

## Description

Sets the data type of the **Tensor** in **TensorDesc**.

## Prototype

```python
set_data_type(data_type: DataType) -> TensorDesc
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| **data_type** | Input | Data type information of the Tensor described by the TensorDesc to be set. |

## Return Value

\(TensorDesc\) Returns itself to support chained calls.

## Constraints

If **data\_type** is not of the **DataType** type, a TypeError is thrown.
