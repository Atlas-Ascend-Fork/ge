# add\_process\_point

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:02:11.817Z pushedAt=2026-10-29T02:54:36.524Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Adds a mapped PP to a FlowNode. Currently, a FlowNode can have only one PP added. After the PP is added, the inputs and outputs of the FlowNode are mapped to those of the PP in order by default.

## Prototype

```python
add_process_point(pp)
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| pp | Union[GraphProcessPoint, FuncProcessPoint] | Processing point mapped to the FlowNode. It can be a [GraphProcessPoint](dataflow-GraphProcessPoint.md) or a [FuncProcessPoint](dataflow-FuncProcessPoint.md). |

## Return Value

Returns None in normal scenarios.

Returns "TypeError" if the parameter type is incorrect.

## Examples

```python
import dataflow as df
pp = df.FuncProcessPoint(...)
flow_node = df.FlowNode(...)
flow_node.add_process_point(pp)
```

## Constraints

The number of inputs and outputs of the added PP is consistent with that of the FlowNode.
