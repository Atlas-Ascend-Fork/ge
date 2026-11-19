# AllocEmptyDataMsg (MetaRunContext Class)

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-13T10:06:19.450Z pushedAt=2026-11-16T06:11:08.672Z nl=2 -->

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
std::shared_ptr<FlowMsg> AllocEmptyDataMsg(MsgType msgType)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| msgType | Input | Message type for which empty data is to be applied. |

## Return Value

- 0: SUCCESS.
- other: FAILED. For details, see [UDF Error Codes](udf-error-code.md).

## Exception Handling

None.

## Constraints

None.
