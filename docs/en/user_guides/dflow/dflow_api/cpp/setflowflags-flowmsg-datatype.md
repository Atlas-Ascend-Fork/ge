# SetFlowFlags (FlowMsg Data Type)

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T12:44:20.534Z pushedAt=2026-10-29T02:15:36.682Z nl=2 -->

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
| flags | Input | flags in the message header.<br>The enumeration values of flags are as follows:<br>enum class FlowFlag : uint32_t {<br>   FLOW_FLAG_EOS = (1U << 0U),  // End-of-stream flag<br>   FLOW_FLAG_SEG = (1U << 1U)  // Segmentation flag for non-contiguous data<br>}; |

## Return Value

None.

## Exception Handling

None.

## Constraints

None.
