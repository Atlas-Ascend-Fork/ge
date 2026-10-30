# AllocEmptyDataMsg (FlowBufferFactory Data Type)

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-21T11:35:49.716Z pushedAt=2026-10-28T12:30:42.435Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Applies for a message of the MsgType type with empty data.

## Prototype

```cpp
FlowMsgPtr AllocEmptyDataMsg(MsgType type)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| type | Input | Message type for which empty data is to be applied for. |

## Return Value

The requested FlowMsg pointer.

## Exception Handling

If the FlowMsg pointer cannot be allocated, NULL is returned.

## Constraints

None.
