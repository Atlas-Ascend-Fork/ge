# placement

<!-- md-trans-meta sourceCommit=1e1ed868460cbcf8fd60b8a11149c952406370ba translatedAt=2026-10-15T04:12:45.820Z pushedAt=2026-10-16T10:53:07.701Z -->

## Applicable Products

Supported on all chips.

## Description

Obtains the storage location of **Tensor** data.

## Prototype

```python
tensor.placement -> TensorPlacement
```

## Parameters

None

## Return Value

Returns `TensorPlacement`, which indicates the location of the data, such as Device HBM, Host, Following, or Device P2P.

## Constraints

The return value is a copy of the **TensorPlacement** enumeration value.

## Examples

```python
placement = x.placement
```
