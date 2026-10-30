# GetTensor (FlowMsg Data Type)

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T12:46:22.248Z pushedAt=2026-10-29T01:47:44.948Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Obtains the Tensor pointer in FlowMsg.

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
