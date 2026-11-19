# set\_flow\_flags

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T09:30:54.378Z pushedAt=2026-11-17T03:39:12.040Z nl=2 -->

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

```python
set_flow_flags(self, flow_flags: int) -> None
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| flow_flags | Input | Flags in the message header.<br>Flags can take the following values:<br>dataflow.flow_func.flow_func.FLOW_FLAG_EOS  // End-of-stream flag<br>dataflow.flow_func.flow_func.FLOW_FLAG_SEG  // Segmentation flag for non-contiguous data |

## Return Value

None

## Exception Handling

None

## Constraints

None
