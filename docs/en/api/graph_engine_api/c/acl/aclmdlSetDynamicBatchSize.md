# aclmdlSetDynamicBatchSize

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:38:35.373Z pushedAt=2026-10-29T03:39:00.351Z nl=2 -->

## Product Support

<!-- npu="950" id525 -->
- Ascend 950PR&950DT products: Supported
<!-- end id525 -->
<!-- npu="A3" id526 -->
- Atlas A3 products: Supported
<!-- end id526 -->
<!-- npu="910b" id527 -->
- Atlas A2 products: Supported
<!-- end id527 -->
<!-- npu="310b" id528 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id528 -->
<!-- npu="310p" id529 -->
- Atlas inference products: Supported
<!-- end id529 -->
<!-- npu="910" id530 -->
- Atlas training products: Supported
<!-- end id530 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlSetDynamicBatchSize_res.md#id1 -->

## Feature Description

In dynamic batch scenarios, call this API before model execution to set the number of images to be processed in each model inference.

## Prototype

```c
aclError aclmdlSetDynamicBatchSize(uint32_t modelId, aclmdlDataset *dataset, size_t index, uint64_t batchSize)
```

## Parameters

| Parameter Name | Input/Output | Description |
| --- | --- | --- |
| modelId | Input | Model ID.<br>After the model is loaded by calling the [aclmdlLoadFromFile](aclmdlLoadFromFile.md) API, the [aclmdlLoadFromMem](aclmdlLoadFromMem.md) API, the [aclmdlLoadFromFileWithMem](aclmdlLoadFromFileWithMem.md) API, or the [aclmdlLoadFromMemWithMem](aclmdlLoadFromMemWithMem.md) API, the model ID is returned. |
| dataset | Input&Output | Pointer to the input data for model inference. For the type definition, see [aclmdlDataset](aclmdlDataset.md).<br>Data of the aclmdlDataset type is used to describe the input data for model inference. The input memory address and memory size are described by data of the aclDataBuffer type. |
| index | Input | Input index that identifies the dynamic batch input. Obtain it by calling the [aclmdlGetInputIndexByName](aclmdlGetInputIndexByName.md) API. The input name is fixed to ACL_DYNAMIC_TENSOR_NAME. |
| batchSize | Input | Number of images processed in each model inference.<br>The batch size set here can only be one of the batch size levels configured during model compilation.<br>If you are not sure about the batch level settings during model compilation, call the [aclmdlGetDynamicBatch](aclmdlGetDynamicBatch.md) API to obtain the number of batch levels supported by the specified model and the batch size of each level. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
