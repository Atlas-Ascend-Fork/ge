# AllocTensorMsg (FlowBufferFactory Data Type)

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-21T11:39:25.905Z pushedAt=2026-10-28T12:30:42.446Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: supported
<!-- end id2 -->

## Function

Applies for a FlowMsg based on the shape, data type, and alignment size.

## Prototype

```cpp
FlowMsgPtr AllocTensorMsg(const std::vector<int64_t> &shape, DataType data_type, uint32_t align = 512U)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| shape | Input | Shape of the Tensor FlowMsg. |
| data_type | Input | DataType of the Tensor FlowMsg. |
| align | Input | Alignment size of the applied memory address. The value range is [32, 64, 128, 256, 512, 1024]. This parameter is currently reserved. |

## Return Value

Pointer to the applied FlowMsg.

## Exception Handling

If the FlowMsg pointer cannot be obtained, NULL is returned.

## Constraints

None.
