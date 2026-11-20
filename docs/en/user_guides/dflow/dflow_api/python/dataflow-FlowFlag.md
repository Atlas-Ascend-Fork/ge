# dataflow.FlowFlag

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T05:13:01.553Z pushedAt=2026-11-16T07:50:03.943Z nl=2 -->

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

Not involved

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
