# shape

<!-- md-trans-meta sourceCommit=1e1ed868460cbcf8fd60b8a11149c952406370ba translatedAt=2026-10-15T04:15:53.211Z pushedAt=2026-10-16T10:53:07.714Z -->

## Applicable Products

Supported on all chips.

## Description

Obtains or sets the **StorageShape** of **TensorDesc**.

## Prototype

```python
desc.shape -> StorageShape
desc.shape = shape
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| shape | Input | Shape of the Tensor, of type `StorageShape` or `List[int]`. |

## Return Value

Returns **StorageShape** when getting; no return value when setting.

## Constraints

- If `shape` is not a StorageShape, a list of integers, or None, a TypeError is thrown.
- The returned StorageShape belongs to the current TensorDesc object and cannot be used after the TensorDesc object is destroyed.

## Examples

```python
shape = desc.shape
desc.shape = [2, 3]
```
