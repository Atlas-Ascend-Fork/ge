# \_\_call\_\_

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:01:42.159Z pushedAt=2026-10-29T02:54:17.757Z nl=2 -->

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

| Exception Information | Meaning |
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
