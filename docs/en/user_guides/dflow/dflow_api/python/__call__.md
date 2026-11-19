# \_\_call\_\_

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T05:05:12.862Z pushedAt=2026-11-16T07:42:14.220Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Calls FlowNode to perform computation.

## Prototype

```python
__call__(*inputs)
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| *inputs | Union[FlowData, FlowOutput] | Dynamic parameters of the [FlowData](dataflow-FlowData.md) or [FlowOutput](dataflow-FlowOutput.md) type. |

## Return Value

Returns a FlowOutput object when there is one output, and a FlowOutput tuple when there are multiple outputs.

The exceptions are as follows.

| Exception Information | Description |
| --- | --- |
| TypeError | Incorrect parameter type |
| ValueError | Incorrect parameter value |

## Examples

```python
import dataflow as df
data = df.FlowData()
flow_node = df.FlowNode(...)
flow_output = flow_node(data)
```

## Constraints

None
