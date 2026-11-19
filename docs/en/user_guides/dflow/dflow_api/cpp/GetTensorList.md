# GetTensorList

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T03:06:24.854Z pushedAt=2026-11-16T03:42:12.226Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Returns the pointer list of all Tensors in FlowMsg.

## Prototype

```cpp
virtual const std::vector<Tensor *> GetTensorList() const
```

## Parameters

None

## Return Value

List of all Tensor pointers.

## Exception Handling

None.

## Constraints

None.
