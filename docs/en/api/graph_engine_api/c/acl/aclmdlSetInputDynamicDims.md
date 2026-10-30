# aclmdlSetInputDynamicDims

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:41:14.290Z pushedAt=2026-10-29T03:41:45.098Z nl=2 -->

## Product Support

<!-- npu="950" id643 -->
- Ascend 950PR&950DT products: Supported
<!-- end id643 -->
<!-- npu="A3" id644 -->
- Atlas A3 products: Supported
<!-- end id644 -->
<!-- npu="910b" id645 -->
- Atlas A2 products: Supported
<!-- end id645 -->
<!-- npu="310b" id646 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id646 -->
<!-- npu="310p" id647 -->
- Atlas inference products: Supported
<!-- end id647 -->
<!-- npu="910" id648 -->
- Atlas training products: Supported
<!-- end id648 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlSetInputDynamicDims_res.md#id1 -->

## Feature Description

If the input Shape of the model is dynamic and the input data Format is ND (ND indicates that any format is supported), call this API to set the specific dimension values for model inference before model execution.

## Prototype

```c
aclError aclmdlSetInputDynamicDims(uint32_t modelId, aclmdlDataset *dataset, size_t index, const aclmdlIODims *dims)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| modelId | Input | Model ID.<br>After a model is loaded successfully by calling the [aclmdlLoadFromFile](aclmdlLoadFromFile.md) API, [aclmdlLoadFromMem](aclmdlLoadFromMem.md) API, [aclmdlLoadFromFileWithMem](aclmdlLoadFromFileWithMem.md) API, or [aclmdlLoadFromMemWithMem](aclmdlLoadFromMemWithMem.md) API, the model ID is returned. |
| dataset | Input&Output | Pointer to the input data for model inference. See [aclmdlDataset](aclmdlDataset.md) for the type definition.<br>Use the aclmdlDataset type to describe the input data for model inference. The input memory address and memory size are described by the aclDataBuffer type. |
| index | Input | Input index that identifies the dynamic dimension.<br>Obtain it by calling the [aclmdlGetInputIndexByName](aclmdlGetInputIndexByName.md) API. The input name is fixed to ACL_DYNAMIC_TENSOR_NAME. |
| dims | Input | Pointer to all dimension information of a specific profile. See [aclmdlIODims](aclmdlIODims.md) for the type definition.<br>The dynamic dimension value set here can only be one of the profiles configured during model compilation.<br>For example, when converting a model with the ATC tool, if input_shape="data:1,1,40,-1;label:1,-1;mask:-1,-1" and dynamic_dims="20,20,1,1; 40,40,2,2; 80,60,4,4", and the actual dimensions of the input data are (1,1,40,20,1,20,1,1), the dims structure is filled as follows (name is not used for now):<br>dims.dimCount = 8<br>dims.dims[0] = 1<br>dims.dims[1] = 1<br>dims.dims[2] = 40<br>dims.dims[3] = 20<br>dims.dims[4] = 1<br>dims.dims[5] = 20<br>dims.dims[6] = 1<br>dims.dims[7] = 1<br>If you are not sure about the dynamic dimension profiles used during model compilation, call the [aclmdlGetInputDynamicDims](aclmdlGetInputDynamicDims.md) API to obtain the number of dynamic dimension profiles supported by the specified model and the values in each profile. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
