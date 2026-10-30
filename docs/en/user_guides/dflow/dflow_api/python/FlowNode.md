# FlowNode

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T12:44:29.101Z pushedAt=2026-10-29T03:10:19.010Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

A compute node in a DataFlow Graph.

## Prototype

```python
FlowNode(input_num, output_num, name=None)
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| input_num | int | Number of inputs of the node. |
| output_num | int | Number of outputs of the node. |
| name | str | Node name. The framework automatically ensures name uniqueness. If not set, names such as FlowNode, FlowNode_1, FlowNode_2, ... are automatically generated. |

## Return Value

Returns None in normal scenarios.

Returns "TypeError" if the parameter type is incorrect.

## Examples

```python
import dataflow as df
flow_node = df.FlowNode(input_num=2, output_num=1)
```

## Constraints

None
