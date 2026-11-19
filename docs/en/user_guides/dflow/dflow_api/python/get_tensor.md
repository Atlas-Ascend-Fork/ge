# get\_tensor

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T05:48:15.588Z pushedAt=2026-11-16T08:02:48.698Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Obtains the tensor object in FlowMsg.

## Prototype

```python
get_tensor() -> dataflow.Tensor
```

## Parameters

None

## Return Value

Returns an object of the dataflow.Tensor type.

## Exception Handling

None

## Constraints

If FlowMsg is empty, tensor returns None.
