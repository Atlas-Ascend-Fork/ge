# set\_flow\_flags

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:14:02.729Z pushedAt=2026-10-29T03:22:09.072Z nl=2 -->

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
