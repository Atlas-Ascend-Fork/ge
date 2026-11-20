# map\_output

<!-- md-trans-meta sourceCommit=2bcdf9e6f737036192b9d97b6ce81ad24f919f18 translatedAt=2026-11-16T08:09:43.986Z pushedAt=2026-11-16T08:44:22.063Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 series products: supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 series products: supported
<!-- end id2 -->

## Function Description

Maps output for a FlowNode, indicating that the pp\_output\_index-th output of pp is given to the node\_output\_index-th output of the FlowNode.

This is an optional call method. If it is not called, the outputs of the FlowNode and pp are mapped in order by default.

## Prototype

```python
map_output(node_output_index, pp, pp_output_index)
```

## Parameters

| Parameter Name | Data Type | Value Description |
| --- | --- | --- |
| node_output_index | int | Output index of the FlowNode. |
| pp | Union[GraphProcessPoint, FuncProcessPoint] | pp mapped to the FlowNode. It can be [GraphProcessPoint](dataflow-GraphProcessPoint.md) or [FuncProcessPoint](dataflow-FuncProcessPoint.md). |
| pp_output_index | int | Output index of pp. |

## Return Value

Returns None in normal scenarios.

In abnormal cases, a DfException is thrown. You can obtain the error\_code and message in DfException by catching the exception to view the specific error code and error information. For details, see [DataFlow Error Codes](dataflow-error-code.md).

## Call Example

```python
import dataflow as df
pp = df.FuncProcessPoint(...)
flow_node = df.FlowNode(input_num=2, output_num=2)
flow_node.add_process_point(pp)
flow_node.map_input(0, pp, 0)
flow_node.map_input(1, pp, 1)
flow_node.map_output(0, pp, 1)
flow_node.map_output(1, pp, 0)
# Build the edge connection
flow_node_out = flow_node(data, data1)
```

## Constraints

None.
