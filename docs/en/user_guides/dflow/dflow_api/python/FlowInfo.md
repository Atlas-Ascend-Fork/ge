# FlowInfo

<!-- md-trans-meta sourceCommit=537d343d7af6b25d23bdc38c9e133ec1c0991c0d translatedAt=2026-11-16T05:28:11.894Z pushedAt=2026-11-16T07:54:48.914Z nl=2 -->

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

Returns "TypeError" if the parameter type is incorrect.

## Examples

```python
import dataflow as df
graph = df.FlowGraph(...)
flow_info = df.FlowInfo(...)
graph.feed_data(...,flow_info)
```

## Constraints

None
