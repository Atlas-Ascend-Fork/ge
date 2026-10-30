# GetTensor (FlowMsg Class)

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T12:45:48.880Z pushedAt=2026-10-29T01:47:37.838Z nl=2 -->

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

Returns a Tensor pointer.

## Exception Handling

None.

## Constraints

Only when the message type is TENSOR\_DATA\_TYPE can a Tensor pointer be obtained; otherwise, NULL is returned.
