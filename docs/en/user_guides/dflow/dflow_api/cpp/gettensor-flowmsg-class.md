# GetTensor (FlowMsg Class)

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T03:05:38.172Z pushedAt=2026-11-16T03:42:03.654Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Gets the Tensor pointer in FlowMsg.

## Prototype

```cpp
Tensor *GetTensor() const
```

## Parameters

None

## Return Value

Returns a Tensor type pointer.

## Exception Handling

None.

## Constraints

A Tensor type pointer can be obtained only when the message type is TENSOR\_DATA\_TYPE; otherwise, NULL is returned.
