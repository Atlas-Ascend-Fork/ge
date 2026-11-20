# SetFlowFlags (FlowMsg Class)

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T03:59:07.470Z pushedAt=2026-11-16T06:04:54.782Z nl=2 -->

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
