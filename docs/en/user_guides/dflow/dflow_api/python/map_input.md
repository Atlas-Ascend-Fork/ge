# map\_input

<!-- md-trans-meta sourceCommit=2bcdf9e6f737036192b9d97b6ce81ad24f919f18 translatedAt=2026-11-16T08:09:52.191Z pushedAt=2026-11-16T08:36:01.651Z nl=2 -->

## Product Support

<!-- npu="A3" id1 -->
- Atlas A3 series: supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 series: supported
<!-- end id2 -->

## Function

Maps inputs to a FlowNode. It indicates that the node\_input\_index-th input of the FlowNode is assigned to the pp\_input\_index-th input of the ProcessPoint, and all attributes in attr are set for this input of the ProcessPoint. This function is optional. If it is not called, the inputs of the FlowNode and the ProcessPoint are mapped in sequence by default.

## Prototype

```python
map_input(node_input_index, pp, pp_input_index, input_attrs=[])
```

## Parameters

| Parameter | Data Type | Description |
| --- | --- | --- |
| node_input_index | int | Input index of the FlowNode, which is less than or equal to the number of inputs. |
| pp | Union[GraphProcessPoint, FuncProcessPoint] | ProcessPoint to which the FlowNode is mapped. It can be [GraphProcessPoint](dataflow-GraphProcessPoint.md) or [FuncProcessPoint](dataflow-FuncProcessPoint.md). |
| pp_input_index | int | Input index of the pp. |
| input_attrs | List[Union[TimeBatch, CountBatch]] | Attribute set. Currently, [TimeBatch](dataflow-TimeBatch.md) and [CountBatch](dataflow-CountBatch.md) are supported. |

## Return Value

None is returned in normal scenarios.

In abnormal scenarios, a DfException is thrown. You can obtain the error\_code and message in the DfException by catching the exception to view the specific error code and error information. For details, see [DataFlow Error Codes](dataflow-error-code.md).

## Example

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

None.
