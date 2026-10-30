# dataflow.FlowFlag

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:09:17.979Z pushedAt=2026-10-29T03:01:39.603Z nl=2 -->

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

Not applicable

## Parameters

The enumeration values are as follows:

- FlowFlag.DATA\_FLOW\_FLAG\_EOS
- FlowFlag.DATA\_FLOW\_FLAG\_SEG

## Return Value

None

## Examples

```python
import dataflow as df
flow_info = df.FlowInfo()
flow_info.flow_flags = df.FlowFlag.DATA_FLOW_FLAG_EOS
```

## Constraints

None
