# SetFlowFlags (FlowMsg Data Type)

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T03:59:31.299Z pushedAt=2026-11-16T06:05:01.554Z nl=2 -->

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
