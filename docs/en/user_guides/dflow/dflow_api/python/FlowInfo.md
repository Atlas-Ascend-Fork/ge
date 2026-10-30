# FlowInfo

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T12:42:34.538Z pushedAt=2026-10-29T03:09:37.938Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Flow information of DataFlow.

## Prototype

```python
FlowInfo(start_time=0, end_time=0, flow_flags=0, transaction_id=0)
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| start_time | int | Start time, in ms. |
| end_time | int | End time, in ms. |
| flow_flags | int | dataflow.FlowFlag. For details, see [dataflow.FlowFlag](dataflow-FlowFlag.md). |
| transaction_id | int | Transaction ID used for DataFlow data transfer. It is valid only when it is not 0. |

## Return Value

None is returned in normal scenarios.

TypeError is returned if the parameter type is incorrect.

## Examples

```python
import dataflow as df
graph = df.FlowGraph(...)
flow_info = FlowInfo(...)
graph.feed_data(...,flow_info)
```

## Constraints

None
