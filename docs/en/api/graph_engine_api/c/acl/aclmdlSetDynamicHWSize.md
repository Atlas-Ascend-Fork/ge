# aclmdlSetDynamicHWSize

<!-- md-trans-meta sourceCommit=aaa82213df16687539020ed5cae4f006363bb63c translatedAt=2026-11-17T14:51:17.174Z pushedAt=2026-11-18T03:38:50.833Z nl=2 -->

## Product Support

<!-- npu="950" id169 -->
- Ascend 950PR&950DT products: Supported
<!-- end id169 -->
<!-- npu="A3" id170 -->
- Atlas A3 products: Supported
<!-- end id170 -->
<!-- npu="910b" id171 -->
- Atlas A2 products: Supported
<!-- end id171 -->
<!-- npu="310b" id172 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id172 -->
<!-- npu="310p" id173 -->
- Atlas inference products: Supported
<!-- end id173 -->
<!-- npu="910" id174 -->
- Atlas training products: Supported
<!-- end id174 -->
<!-- npu="IPV350" id175 -->
- IPV350: not supported
<!-- end id175 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlSetDynamicHWSize_res.md#id1 -->

## Function Description

In dynamic resolution scenarios, call this API before om or om2 model execution to set the height and width of the input image during model inference.

## Prototype

```c
aclError aclmdlSetDynamicHWSize(uint32_t modelId, aclmdlDataset *dataset, size_t index, uint64_t height, uint64_t width)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| modelId | Input | Model ID.<br>After a model is loaded successfully by calling the [aclmdlLoadFromFile](aclmdlLoadFromFile.md) API, [aclmdlLoadFromMem](aclmdlLoadFromMem.md) API, [aclmdlLoadFromFileWithMem](aclmdlLoadFromFileWithMem.md) API, or [aclmdlLoadFromMemWithMem](aclmdlLoadFromMemWithMem.md) API, the model ID is returned. |
| dataset | Input&Output | Pointer to the input data for model inference. See [aclmdlDataset](aclmdlDataset.md) for the type definition.<br>Use the aclmdlDataset type to describe the input data for model inference. The input memory address and memory size are described by the aclDataBuffer type. |
| index | Input | Input index that identifies the dynamic resolution input. It must be obtained by calling the [aclmdlGetInputIndexByName](aclmdlGetInputIndexByName.md) API. The input name is fixed to ACL_DYNAMIC_TENSOR_NAME. |
| height | Input | H value to set.<br>The resolution set here can only be one of the resolution levels configured during model compilation.<br>If you are not sure about the resolution levels configured during model compilation, call the [aclmdlGetDynamicHW](aclmdlGetDynamicHW.md) API to obtain the number of resolution levels supported by the specified model and the width and height of each level. |
| width | Input | W value to set.<br>The resolution set here can only be one of the resolution levels configured during model compilation.<br>If you are not sure about the resolution levels configured during model compilation, call the [aclmdlGetDynamicHW](aclmdlGetDynamicHW.md) API to obtain the number of resolution levels supported by the specified model and the width and height of each level. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
