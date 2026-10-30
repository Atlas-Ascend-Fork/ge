# GraphProcessPoint

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:02:19.841Z pushedAt=2026-10-29T03:15:18.380Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

GraphProcessPoint constructor, which returns a GraphProcessPoint object.

## Prototype

```python
GraphProcessPoint(framework, graph_file, load_params={}, compile_config_path="", name=None)
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| framework | Framework | Framework type of the IR file. For details, see [dataflow.Framework](dataflow-Framework.md). |
| graph_file | str | Path of the IR file. |
| load_params | Dict[str, str] | Map of configuration parameters. The key is the parameter type and the value is the parameter value, both in String format. It is used to describe the parsing parameters of the original model. |
| compile_config_path | str | Path of the configuration file used when compiling the graph.<br>The following is an example of the configuration file:<br>{"build_option":{},"inputs_tensor_desc":[{"data_type":"DT_UINT32","shape":[3]},{"data_type":"DT_UINT32","shape":[3]}]} |
| name | str | Name of the processing point. The framework automatically ensures the uniqueness of the name. If it is not set, names such as GraphProcessPoint, GraphProcessPoint_1, GraphProcessPoint_2, ... are automatically generated. |

**Table 1** JSON configuration file of GraphProcessPoint

| Configuration Item | Optional/Mandatory | Description |
| --- | --- | --- |
| build_options | Optional | The value is map<string, string>. When it needs to be set, refer to build_options in Ascend Graph. |
| inputs_tensor_desc | Optional | The value is a list. It specifies the input nodes of the Graph, and the list elements are tensor descriptions. |
| inputs_tensor_desc.data_type | Optional | String type.<br>The value is the string corresponding to data_type in the Graph. |
| inputs_tensor_desc.shape | Optional | The value is a list of integers.<br>The value is the shape in the Graph. |

## Return Value

None is returned in normal scenarios.

In abnormal scenarios, a DfException is thrown. You can catch the exception to obtain the error\_code and message in DfException to view the specific error code and error message. For details, see [DataFlow Error Codes](dataflow-error-code.md).

## Examples

```python
import dataflow as df
pp1 = df.GraphProcessPoint(...)
```

## Constraints

None
