# fnode

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T05:30:41.737Z pushedAt=2026-11-16T07:56:24.095Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Generates a FlowNode based on the current GraphProcessPoint and returns a FlowNode object.

## Prototype

```python
fnode(input_num, output_num, name=None)
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| input_num | int | Number of inputs of the node. |
| output_num | int | Number of outputs of the node. |
| name | str | Name of the node. The framework automatically ensures name uniqueness. If not set, names such as FlowNode, FlowNode_1, FlowNode_2, ... are automatically generated. |

## Return Value

Returns a FlowNode object.

## Examples

```python
import dataflow as df
pp1 = df.GraphProcessPoint(...)
flow_node = pp1.fnode(input_num=2, output_num=1)
```

## Constraints

None
