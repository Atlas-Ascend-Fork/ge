# storage\_shape

<!-- md-trans-meta sourceCommit=1e1ed868460cbcf8fd60b8a11149c952406370ba translatedAt=2026-10-15T04:14:28.653Z pushedAt=2026-10-16T10:53:07.708Z -->

## Applicable Products

Supported on all chips.

## Description

Obtains the runtime storage shape of **Tensor**.

## Prototype

```python
tensor.storage_shape -> Shape
```

## Parameters

None

## Return Value

Returns a `Shape` object that represents the actual storage shape. This object is provided by the runtime and is valid only during the current execution callback.

## Constraints

- The returned object can be used only during the current execution callback.
- `storage_shape` may differ from [`origin_shape`](origin_shape.md) due to format conversion or alignment.

## Examples

```python
storage_shape = x.storage_shape
```
