# SetFlowFlags (FlowMsg Class)

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T12:43:49.484Z pushedAt=2026-10-29T02:15:30.325Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Sets the flags in the FlowMsg message header.

## Prototype

```cpp
void SetFlowFlags(uint32_t flags)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| flags | Input | Flags in the message header.<br>flags can take the following enumeration values:<br>enum class FlowFlag : uint32_t {<br>   FLOW_FLAG_EOS = (1U << 0U),  // End-of-stream flag<br>   FLOW_FLAG_SEG = (1U << 1U)  // Segment flag for non-contiguous data<br>}; |

## Return Value

None.

## Exception Handling

None.

## Constraints

None.
