# origin\_shape

<!-- md-trans-meta sourceCommit=1e1ed868460cbcf8fd60b8a11149c952406370ba translatedAt=2026-10-15T04:12:01.217Z pushedAt=2026-10-16T10:53:07.698Z -->

## Applicable Products

Supported on all chips.

## Description

Obtains the original logical shape of a **Tensor**.

## Prototype

```python
tensor.origin_shape -> Shape
```

## Parameters

None

## Return Value

Returns a `Shape` object that represents the original logical shape. This object is provided by the runtime and is valid only during the current execution callback.

## Constraints

- The returned object can only be used during the current execution callback.
- When the actual memory layout at runtime is required, use [`storage_shape`](storage_shape.md).

## Examples

```python
origin_shape = x.origin_shape
```
