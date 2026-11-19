# flow\_flags

<!-- md-trans-meta sourceCommit=537d343d7af6b25d23bdc38c9e133ec1c0991c0d translatedAt=2026-11-16T05:27:15.566Z pushedAt=2026-11-16T07:54:20.698Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Reads and sets the flow\_flags of FlowInfo through the property method.

## Prototype

```python
@property
def flow_flags(self)
@flow_flags.setter
def flow_flags(self, new_value)
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| new_value | int | New value of flow_flags to be reset. |

## Return Value

flow\_flags property.

## Examples

```python
import dataflow as df
graph = df.FlowGraph(...)
flowinfo = df.FlowInfo(...)
flowinfo.flow_flags = df.FlowFlag.DATA_FLOW_FLAG_EOS | df.FlowFlag.DATA_FLOW_FLAG_SEG
print(flowinfo.flow_flags)
```

## Constraints

None
