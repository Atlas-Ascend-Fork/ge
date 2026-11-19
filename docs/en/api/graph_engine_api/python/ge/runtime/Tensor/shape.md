# shape

<!-- md-trans-meta sourceCommit=1e1ed868460cbcf8fd60b8a11149c952406370ba translatedAt=2026-10-15T04:13:29.684Z pushedAt=2026-10-16T10:53:07.703Z -->

## Product Support

Supported on all chips.

## Description

Obtains the **Shape** description of a **Tensor**, which stores both **OriginShape** and **StorageShape**.

## Prototype

```python
tensor.shape -> StorageShape
```

## Parameters

None

## Return Value

Returns the `StorageShape` object of the current Tensor. This object is provided by the runtime and is valid only during the current execution callback.

## Constraints

- The returned object can only be used during the current execution callback.
- The `origin_shape` of the returned object represents the original logical shape, and `storage_shape` represents the runtime storage shape.

## Examples

```python
storage_shape = x.shape
```
