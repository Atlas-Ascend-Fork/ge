# aclmdlGetCurOutputDims

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:10:05.588Z pushedAt=2026-10-29T03:10:50.801Z nl=2 -->

## Product Support

<!-- npu="950" id727 -->
- Ascend 950PR&950DT products: Supported
<!-- end id727 -->
<!-- npu="A3" id728 -->
- Atlas A3 products: Supported
<!-- end id728 -->
<!-- npu="910b" id729 -->
- Atlas A2 products: Supported
<!-- end id729 -->
<!-- npu="310b" id730 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id730 -->
<!-- npu="310p" id731 -->
- Atlas inference products: Supported
<!-- end id731 -->
<!-- npu="910" id732 -->
- Atlas training products: Supported
<!-- end id732 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlGetCurOutputDims_res.md#id1 -->

## Feature Description

Obtains the actual dimension information of a specified model output tensor based on the model description.

## Prototype

```c
aclError aclmdlGetCurOutputDims(const aclmdlDesc *modelDesc, size_t index, aclmdlIODims *dims)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| modelDesc | Input | Pointer to the aclmdlDesc type.<br>Call the [aclmdlCreateDesc](aclmdlCreateDesc.md) API in advance to create data of the aclmdlDesc type. |
| index | Input | Specifies which output's Dims to obtain. The index value starts from 0. |
| dims | Output | Pointer to the output actual dimension information. See [aclmdlIODims](aclmdlIODims.md) for the type definition.<br>If the tensor name length is greater than 127, when outputting dims.name, the system converts the tensor name to the "acl_modelId_${id}_output_${index}_${random string}" format (if the converted tensor name conflicts with an existing tensor name in the model, "_${random string}" is appended to the end of the converted name; otherwise, no random string is appended), and establishes a mapping between the converted name and the original name. You can call the [aclmdlGetTensorRealName](aclmdlGetTensorRealName.md) API and pass the converted name to obtain the original name (if you pass the original name to the API, the original name is still returned). If the tensor name length is less than or equal to 127, the tensor name is output as-is when outputting dims.name. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).

## Constraints

Currently, this API can be used to obtain the dimension information of model output tensors only in the following scenarios:

- When multiple batch sizes, resolutions, or dimension values are configured during model conversion to implement dynamic batch, dynamic resolution, or dynamic dimensions (ND format), if you have called [aclmdlSetDynamicBatchSize](aclmdlSetDynamicBatchSize.md) to set the batch, or called [aclmdlSetDynamicHWSize](aclmdlSetDynamicHWSize.md) to set the width and height of the input image, or called [aclmdlSetInputDynamicDims](aclmdlSetInputDynamicDims.md) to set the value of a dynamic dimension, you can use this API to obtain the actual dimension information of the specified model output tensor. If you have not called [aclmdlSetDynamicBatchSize](aclmdlSetDynamicBatchSize.md), [aclmdlSetDynamicHWSize](aclmdlSetDynamicHWSize.md), or [aclmdlSetInputDynamicDims](aclmdlSetInputDynamicDims.md), this API returns the dimension information of the maximum profile.
- In fixed-shape scenarios, this API obtains the dimension information of the specified model output tensor.
