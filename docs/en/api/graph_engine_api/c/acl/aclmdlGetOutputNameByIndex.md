# aclmdlGetOutputNameByIndex

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:23:41.712Z pushedAt=2026-10-29T03:23:11.253Z nl=2 -->

## Product Support

<!-- npu="950" id748 -->
- Ascend 950PR&950DT products: Supported
<!-- end id748 -->
<!-- npu="A3" id749 -->
- Atlas A3 products: Supported
<!-- end id749 -->
<!-- npu="910b" id750 -->
- Atlas A2 products: Supported
<!-- end id750 -->
<!-- npu="310b" id751 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id751 -->
<!-- npu="310p" id752 -->
- Atlas inference products: Supported
<!-- end id752 -->
<!-- npu="910" id753 -->
- Atlas training products: Supported
<!-- end id753 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlGetOutputNameByIndex_res.md#id1 -->

## Feature Description

Obtains the output operator name, the edge index of the operator output, the top name, or the output name of a specified output in the model based on the model description.

## Prototype

```c
const char *aclmdlGetOutputNameByIndex(const aclmdlDesc *modelDesc, size_t index)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| modelDesc | Input | Pointer to the aclmdlDesc type.<br>Call [aclmdlCreateDesc](aclmdlCreateDesc.md) in advance to create the aclmdlDesc type data. |
| index | Input | Specifies the output operator name, operator output edge index, top name, or output name of the output to obtain. The value of index starts from 0. |

## Return Value

Returns the output operator name, operator output edge index, top name, or output name of the specified output. The return value format obtained by calling this API varies depending on the original network and the model construction method.

- Caffe network

    The return value format is as follows, with items separated by colons. If the model contains a top name, it is returned; otherwise, it is not returned:

    output operator name : operator output edge index : top name

- TensorFlow network
  - When the om model is built using the ATC tool, the return value format is as follows, with items separated by colons:

    output operator name : operator output edge index

  - When the om model is built using the graph construction API, the return value format is as follows, with items separated by underscores:

    output_network output index_output operator name_operator output edge index

    For details about the graph construction API, see *[Graph Development](../../../../user_guides/graph_dev/compile_and_run_graph/compile_graph_to_offline_model.md)*.

- ONNX network
  - When building the model, if you do not specify the output node name (node\_name) or the output name (the name of output), or if you specify only the output name, the return value format is as follows, with items separated by colons:

    output operator name : operator output edge index : output name

  - When building the model, if you specify the output node name (node\_name):

    The output operator name may be the operator name after graph fusion, or the subgraph name.

    - When the om model is built using the ATC tool, the return value format is as follows, with items separated by colons:

        output operator name : operator output edge index

    - When the om model is built using the graph construction API, the return value format is as follows, with items separated by underscores:

        output_network output index_output operator name_operator output edge index

        For details about the graph construction API, see *[Graph Development](../../../../user_guides/graph_dev/compile_and_run_graph/compile_graph_to_offline_model.md)*.

  - If both the output node name (node\_name) and the output name (the name of output) are specified, the API returns an error.
