# AllocEmptyDataMsg (MetaRunContext Class)

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-21T11:36:33.103Z pushedAt=2026-10-28T12:30:42.437Z nl=2 -->

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
