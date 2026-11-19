# origin\_format

<!-- md-trans-meta sourceCommit=1e1ed868460cbcf8fd60b8a11149c952406370ba translatedAt=2026-10-15T04:11:47.219Z pushedAt=2026-10-16T10:53:07.696Z -->

## Applicable Products

Supported on all chips.

## Description

Obtains the original **Format** of the **Tensor**.

## Prototype

```python
tensor.origin_format -> Format
```

## Parameters

None

## Return Value

Returns the `Format` that represents the original logical layout.

## Constraints

The return value is a copy of the `Format` enumeration value and can be saved outside the current callback; however, it should not be used to extend the lifecycle of the `Tensor`.

## Examples

```python
origin_format = x.origin_format
```
