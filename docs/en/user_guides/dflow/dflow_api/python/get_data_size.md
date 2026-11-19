# get\_data\_size

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T05:38:14.013Z pushedAt=2026-11-16T08:01:20.445Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Obtains the data size of a Tensor.

## Prototype

```python
get_data_size(self) -> int
```

## Parameters

None

## Return Value

Returns the data size of the Tensor.

## Exception Handling

None

## Constraints

Modifying the returned TensorDesc information does not affect the existing TensorDesc information in the Tensor object.
