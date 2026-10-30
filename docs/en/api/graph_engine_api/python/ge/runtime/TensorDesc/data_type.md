# data\_type

<!-- md-trans-meta sourceCommit=1e1ed868460cbcf8fd60b8a11149c952406370ba translatedAt=2026-10-15T04:15:07.359Z pushedAt=2026-10-16T10:53:07.712Z -->

## Applicable Products

Supported on all chips.

## Description

Obtains or sets the **DataType** of **TensorDesc**.

## Prototype

```python
desc.data_type -> DataType
desc.data_type = data_type
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| data\_type | Input | Data type of the **Tensor**, of the type **DataType**. |

## Return Value

Returns **DataType** when getting; no return value when setting.

## Constraints

- If `data_type` is not of the DataType type, a TypeError is thrown.
- If `data_type` equals DataType.DT_MAX, a ValueError is thrown.

## Example

```python
data_type = desc.data_type
desc.data_type = DataType.DT_FLOAT16
```
