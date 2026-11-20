# FlowGraph

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T05:28:15.811Z pushedAt=2026-11-16T07:54:42.292Z nl=2 -->

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

In abnormal scenarios, a DfException is thrown. You can catch the exception to obtain the error\_code and message in DfException to view the specific error code and error information. For details, see [DataFlow Error Codes](dataflow-error-code.md).

## Examples

```python
import dataflow as df
graph = df.FlowGraph(...)
```

## Constraints

None
