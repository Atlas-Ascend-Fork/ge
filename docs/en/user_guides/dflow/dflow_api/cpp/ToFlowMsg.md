# ToFlowMsg

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T04:12:20.953Z pushedAt=2026-11-16T06:11:43.981Z nl=2 -->

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
