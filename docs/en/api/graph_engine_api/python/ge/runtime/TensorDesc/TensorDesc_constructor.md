# TensorDesc Constructor

<!-- md-trans-meta sourceCommit=1e1ed868460cbcf8fd60b8a11149c952406370ba translatedAt=2026-10-15T04:15:59.560Z pushedAt=2026-10-16T10:53:07.715Z -->

## Applicable Products

Supported on all chips.

## Description

Creates a `TensorDesc` for metadata inference of Python custom operators.

## Prototype

```python
TensorDesc(shape: Optional[Union[StorageShape, List[int]]], data_type: DataType)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| shape | Input | Shape of the Tensor, of the type `StorageShape` or `List[int]`. `None` indicates a scalar. |
| data_type | Input | Data type of the Tensor, of the type `DataType`. |

## Return Value

Returns a **TensorDesc** object.

## Constraints

- A `TypeError` is raised when `shape` is not `StorageShape`, a list of integers, or `None`.
- A `TypeError` or `ValueError` is raised when `data_type` is not `DataType` or equals `DataType.DT_MAX`.

## Examples

```python
from ge.graph import DataType
from ge.runtime import TensorDesc

desc = TensorDesc([2, 3], DataType.DT_FLOAT)
```
