# AllocRawDataMsg (FlowBufferFactory Data Type)

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-21T11:37:18.328Z pushedAt=2026-10-28T12:30:42.439Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Applies for a contiguous memory block based on the input size to hold raw data.

## Prototype

```cpp
FlowMsgPtr AllocRawDataMsg(size_t size, uint32_t align = 512U)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| size | Input | Size of the memory to be applied for. |
| align | Input | Alignment size of the memory address to be applied for. The value range is [32, 64, 128, 256, 512, 1024].<br>This parameter is reserved and its value is not validated. |

## Return Value

Pointer to the allocated FlowMsg.

## Exception Handling

If no FlowMsg pointer can be obtained, NULL is returned.

## Constraints

None.
