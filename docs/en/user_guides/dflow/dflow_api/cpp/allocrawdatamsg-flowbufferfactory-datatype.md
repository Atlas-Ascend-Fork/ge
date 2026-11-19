# AllocRawDataMsg (FlowBufferFactory Data Type)

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-13T10:06:36.293Z pushedAt=2026-11-16T06:11:08.673Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Allocates a contiguous buffer based on the input size to carry activation of the raw data type.

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

Pointer to the applied FlowMsg.

## Exception Handling

If the FlowMsg pointer cannot be obtained, NULL is returned.

## Constraints

None.
