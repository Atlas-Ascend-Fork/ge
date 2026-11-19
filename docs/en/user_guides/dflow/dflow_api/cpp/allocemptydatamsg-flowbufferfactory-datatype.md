# AllocEmptyDataMsg (FlowBufferFactory Data Type)

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-13T10:05:30.948Z pushedAt=2026-11-16T06:11:08.668Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Allocates a message of the MsgType type for empty data.

## Prototype

```cpp
FlowMsgPtr AllocEmptyDataMsg(MsgType type)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| type | Input | Message type for which empty data is to be applied. |

## Return Value

Pointer to the applied FlowMsg.

## Exception Handling

If the FlowMsg pointer cannot be obtained, NULL is returned.

## Constraints

None.
