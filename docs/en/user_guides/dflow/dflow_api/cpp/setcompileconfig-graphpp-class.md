# SetCompileConfig (GraphPp Class)

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T12:39:37.406Z pushedAt=2026-10-29T02:14:18.002Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Sets the path and file name of the JSON configuration file of GraphPp. The configuration file is used for the description and compilation of AscendGraph.

Returns the configured GraphPp.

## Prototype

```cpp
GraphPp &SetCompileConfig(const char_t *json_file_path)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| json_file_path | Input | Path and file name of the JSON configuration file of GraphPp.<br>The JSON configuration file of GraphPp is used for the description and compilation of AscendGraph.<br>An example is as follows. For parameter descriptions, see Table 1.<br>{"build_option":{},"inputs_tensor_desc":[{"data_type":"DT_UINT32","shape":[3]},{"data_type":"DT_UINT32","shape":[3]}]} |

**Table 1** JSON configuration file of GraphPp

| Configuration Item | Optional/Required | Description |
| --- | --- | --- |
| build_option | Optional | The value is map<string, string>. When configuration is required, refer to build_option in Ascend Graph. |
| inputs_tensor_desc | Optional | The value is a list. It specifies the input nodes of Ascend Graph, and each list element is the description of a Tensor. |
| inputs_tensor_desc.data_type | Optional | String type.<br>The value is the string corresponding to data_type in Ascend Graph. |
| inputs_tensor_desc.shape | Optional | The value is a list of integers.<br>The value is the shape in Ascend Graph. |

## Return Value

Returns the configured GraphPp.

## Exception Handling

None.

## Constraints

None.
