# map\_input

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:03:32.691Z pushedAt=2026-10-29T03:15:52.348Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Maps the input of a FlowNode, indicating that the node\_input\_index-th input of the FlowNode is passed to the pp\_input\_index-th input of the ProcessPoint, and sets all properties in attr for this input of the ProcessPoint. Returns the FlowNode node after mapping. This function is optional. If it is not called, the inputs of the FlowNode and the ProcessPoint are mapped in order by default.

## Prototype

```python
map_input(node_input_index, pp, pp_input_index, input_attrs=[])
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| node_input_index | int | Input index of the FlowNode node, which must be less than or equal to the number of inputs. |
| pp | Union[GraphProcessPoint, FuncProcessPoint] | ProcessPoint to which the FlowNode node is mapped. It can be [GraphProcessPoint](dataflow-GraphProcessPoint.md) or [FuncProcessPoint](dataflow-FuncProcessPoint.md). |
| pp_input_index | int | Input index of the pp. |
| input_attrs | List[Union[TimeBatch, CountBatch]] | Attribute set. Currently, [TimeBatch](dataflow-TimeBatch.md) and [CountBatch](dataflow-CountBatch.md) are supported. |

## Return Value

Returns None in normal scenarios.

In the following exceptional cases, a DfException is thrown. You can catch the exception and obtain the error\_code and message in DfException to view the specific error code and error information. For details, see [DataFlow Error Codes](dataflow-error-code.md).

## Examples

```python
import dataflow as df
pp = df.FuncProcessPoint(...)
flow_node = df.FlowNode(input_num=2, output_num=1)
flow_node.add_process_point(pp)
flow_node.map_input(0, pp, 1)
flow_node.map_input(1, pp, 0)
flow_node_out = flow_node(data0, data1)
```

## Constraints

None
