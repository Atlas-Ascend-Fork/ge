# add\_process\_point

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T05:05:13.512Z pushedAt=2026-11-16T07:43:37.367Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Adds a mapped pp to a FlowNode. A FlowNode can add only one pp. After the pp is added, the input/output of the FlowNode and the input/output of the pp are mapped in sequence by default.

## Prototype

```python
add_process_point(pp)
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| pp | Union[GraphProcessPoint, FuncProcessPoint] | Processing point mapped to the FlowNode. It can be a [GraphProcessPoint](dataflow-GraphProcessPoint.md) or a [FuncProcessPoint](dataflow-FuncProcessPoint.md). |

## Return Value

None is returned in normal scenarios.

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
