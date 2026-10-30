# ToFlowMsg (Raw Data)

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T12:55:46.032Z pushedAt=2026-10-29T02:40:38.351Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Converts the input raw data into a FlowMsg that carries the raw data.

## Prototype

```cpp
FlowMsgPtr ToFlowMsg(const RawData &raw_data)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| raw_data | Input | RawData object. |

## Return Value

Pointer to the converted FlowMsg.

## Exception Handling

NULL is returned if the conversion fails.

## Constraints

None.
