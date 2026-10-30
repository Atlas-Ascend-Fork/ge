# aclmdlGetDatasetTensorDesc

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:11:27.981Z pushedAt=2026-10-29T03:11:17.408Z nl=2 -->

## Product Support

<!-- npu="950" id1133 -->
- Ascend 950PR&950DT products: Supported
<!-- end id1133 -->
<!-- npu="A3" id1134 -->
- Atlas A3 products: Supported
<!-- end id1134 -->
<!-- npu="910b" id1135 -->
- Atlas A2 products: Supported
<!-- end id1135 -->
<!-- npu="310b" id1136 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id1136 -->
<!-- npu="310p" id1137 -->
- Atlas inference products: Supported
<!-- end id1137 -->
<!-- npu="910" id1138 -->
- Atlas training products: Supported
<!-- end id1138 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlGetDatasetTensorDesc_res.md#id1 -->

## Feature Description

If the shape of the model input or output is dynamic, after model execution you can call this API to obtain the tensor description information of a specified input or output from data of the aclmdlDataset type (which is used to describe the input data and output data during model inference).

**Typical scenario**: If the model input shape is dynamic, call [aclmdlSetDatasetTensorDesc](aclmdlSetDatasetTensorDesc.md) to set the tensor description information of the input before model execution, and after model execution, call the aclmdlGetDatasetTensorDesc API to obtain the tensor description information of the dynamic model output.

## Prototype

```c
aclTensorDesc *aclmdlGetDatasetTensorDesc(const aclmdlDataset *dataset, size_t index)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| dataset | Input | Pointer to the input or output data of model execution.<br>Call the [aclmdlCreateDataset](aclmdlCreateDataset.md) API in advance to create data of the aclmdlDataset type. |
| index | Input | Sequence number of the input or output.<br>When a model has multiple inputs and outputs, to avoid sequence number errors, call the [aclmdlGetInputNameByIndex](aclmdlGetInputNameByIndex.md) and [aclmdlGetOutputNameByIndex](aclmdlGetOutputNameByIndex.md) APIs first to obtain the input and output names, and then set the index based on the index corresponding to the input and output names. |

## Return Value

Returns the tensor description information of the specified input or output, of the [aclTensorDesc](aclTensorDesc.md) type.
