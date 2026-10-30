# map\_output

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:03:42.349Z pushedAt=2026-10-29T03:16:08.436Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Maps the output of a FlowNode, indicating that the pp\_output\_index-th output of pp is passed to the node\_output\_index-th output of the FlowNode, and returns the FlowNode node after mapping.

This is an optional method. If it is not called, the outputs of the FlowNode and pp are mapped in sequence by default.

## Prototype

```python
map_output(node_output_index, pp, pp_output_index)
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| node_output_index | int | Output index of the FlowNode. |
| pp | Union[GraphProcessPoint, FuncProcessPoint] | pp to which the FlowNode node is mapped. It can be [GraphProcessPoint](dataflow-GraphProcessPoint.md) or [FuncProcessPoint](dataflow-FuncProcessPoint.md). |
| pp_output_index | int | Output index of pp. |

## Return Value

Returns None in normal scenarios.

In the following exception scenarios, a DfException is thrown. You can catch the exception to obtain the error\_code and message in DfException to view the specific error code and error information. For details, see [DataFlow Error Codes](dataflow-error-code.md).

## Examples

```python
import dataflow as df
pp = df.FuncProcessPoint(...)
flow_node = df.FlowNode(input_num=2, output_num=2)
flow_node.add_process_point(pp)
flow_node.map_input(0, pp, 0)
flow_node.map_input(1, pp, 1)
flow_node.map_output(0, pp, 1)
flow_node.map_output(1, pp, 0)
# Build the edge relationship
flow_node_out = flow_node(data, data1)
```

## Constraints

None
