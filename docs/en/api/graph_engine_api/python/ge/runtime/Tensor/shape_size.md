# shape\_size

<!-- md-trans-meta sourceCommit=1e1ed868460cbcf8fd60b8a11149c952406370ba translatedAt=2026-10-15T04:13:13.053Z pushedAt=2026-10-16T10:53:07.702Z -->

## Applicable Products

Supported on all chips.

## Description

Obtains the number of elements represented by the **StorageShape** of the Tensor.

## Prototype

```python
tensor.shape_size -> int
```

## Parameters

None

## Return Value

Returns the number of elements in **StorageShape**.

## Constraints

This value is calculated based on StorageShape and cannot be used in place of `size` to obtain the number of bytes.

## Examples

```python
element_count = x.shape_size
```
