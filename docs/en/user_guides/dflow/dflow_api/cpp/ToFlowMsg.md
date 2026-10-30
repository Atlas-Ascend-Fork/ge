# ToFlowMsg

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T12:56:30.438Z pushedAt=2026-10-29T02:40:49.536Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Converts the input Tensor into a FlowMsg that carries the Tensor.

## Prototype

```cpp
virtual std::shared_ptr<FlowMsg> ToFlowMsg(std::shared_ptr<Tensor> tensor)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| tensor | Input | Pointer to the Tensor. |

## Return Value

Pointer to the converted FlowMsg.

## Exception Handling

Returns NULL if the conversion fails.

## Constraints

None.
