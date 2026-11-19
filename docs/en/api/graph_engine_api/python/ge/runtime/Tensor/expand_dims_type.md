# expand\_dims\_type

<!-- md-trans-meta sourceCommit=1e1ed868460cbcf8fd60b8a11149c952406370ba translatedAt=2026-10-15T04:10:34.221Z pushedAt=2026-10-16T10:53:07.693Z -->

## Applicable Products

Supported on all chips.

## Description

Obtains the dimension expansion rule used when converting **OriginShape** to **StorageShape**.

## Prototype

```python
tensor.expand_dims_type -> ExpandDimsType
```

## Parameters

None

## Return Value

Returns the `ExpandDimsType` object in the current Tensor Format. This object is provided by the runtime and is valid only during the current execution callback.

## Constraints

- The returned object can only be used during the current execution callback.
- This rule describes the expanded dimensions introduced by the format and does not indicate a data copy or memory allocation operation.

## Examples

```python
expand_dims_type = x.expand_dims_type
```
