# TensorDesc Constructor

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T03:29:23.891Z pushedAt=2026-10-16T10:53:07.538Z -->

## Applicable Products

Supported on all chips.

## Description

Creates a new **TensorDesc** object.

## Prototype

```python
TensorDesc(shape: Optional[List[int]] = None, format: Format = Format.FORMAT_ND, data_type: DataType = DataType.DT_FLOAT)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| shape | Input | Tensor shape, of type **list[int]**, which can be a **Shape** object. **None** indicates a scalar. |
| format | Input | Tensor data format, of type **Format**, with a default value of **Format.FORMAT_ND**. |
| data_type | Input | Tensor data type, of type **DataType**, with a default value of **DataType.DT_FLOAT**. |

## Return Value

Returns a **TensorDesc** object.

## Constraints

- If **format** is not of the **Format** type, a **TypeError** is thrown.
- If data\_type is not of the **DataType** type, a **TypeError** is thrown.
