# FlowGraph

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T12:42:34.300Z pushedAt=2026-10-29T03:07:40.243Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

A DataFlow graph, which consists of input nodes (FlowData) and compute nodes (FlowNode).

## Prototype

```python
FlowGraph(outputs, graph_options={}, name=None, graphpp_builder_async = False)
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| outputs | List[FlowOutput] | All outputs of the FlowGraph output nodes. For details, see [dataflow.FlowOutput](dataflow-FlowOutput.md). |
| graph_options | Dict[str, str] | Compilation options of FlowGraph. The configuration parameters passed to the AddGraph API are currently supported.<br>Adapt the configuration examples to the Python language. |
| name | str | Graph name. The framework automatically ensures name uniqueness. If not set, names such as FlowGraph, FlowGra, FlowGra, ... are automatically generated. |
| graphpp_builder_async | bool | Whether the builder of GraphProcessPoint in FlowGraph is executed asynchronously. The values are as follows:<br><br>  - True: yes<br>  - False: no<br><br>Default value: False |

## Return Value

None is returned in normal scenarios.

In abnormal scenarios, a DfException is thrown. You can catch the exception to obtain the error\_code and message in DfException to view the specific error code and error message. For details, see [DataFlow Error Codes](dataflow-error-code.md).

## Examples

```python
import dataflow as df
graph = df.FlowGraph(...)
```

## Constraints

None
