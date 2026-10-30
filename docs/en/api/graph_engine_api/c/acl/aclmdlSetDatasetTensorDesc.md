# aclmdlSetDatasetTensorDesc

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:38:10.197Z pushedAt=2026-10-29T03:38:51.743Z nl=2 -->

## Product Support

<!-- npu="950" id379 -->
- Ascend 950PR&950DT products: Supported
<!-- end id379 -->
<!-- npu="A3" id380 -->
- Atlas A3 products: Supported
<!-- end id380 -->
<!-- npu="910b" id381 -->
- Atlas A2 products: Supported
<!-- end id381 -->
<!-- npu="310b" id382 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id382 -->
<!-- npu="310p" id383 -->
- Atlas inference products: Supported
<!-- end id383 -->
<!-- npu="910" id384 -->
- Atlas training products: Supported
<!-- end id384 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlSetDatasetTensorDesc_res.md#id1 -->

## Feature Description

If the Shape of a model input or output is dynamic, call this API to set the tensor description of the model input or output before model execution.

## Prototype

```c
aclError aclmdlSetDatasetTensorDesc(aclmdlDataset *dataset, aclTensorDesc *tensorDesc, size_t index)
```

## Parameters

| Parameter Name | Input/Output | Description |
| --- | --- | --- |
| dataset | Output | Pointer to the aclmdlDataset address to which the aclTensorDesc is to be added, representing the input or output data structure for model execution.<br>Call [aclmdlCreateDataset](aclmdlCreateDataset.md) in advance to create data of the aclmdlDataset type, and then call [aclmdlAddDatasetBuffer](aclmdlAddDatasetBuffer.md) to add aclDataBuffer to aclmdlDataset. |
| tensorDesc | Input | Pointer to the aclTensorDesc address to be added, representing the tensor description of the corresponding input or output during model execution.<br>Call [aclCreateTensorDesc](aclCreateTensorDesc.md) in advance to create data of the aclTensorDesc type. Currently, only the dimension information in the model input and output tensor description is valid (corresponding to the numDims parameter representing the number of dimensions and the dims parameter representing the dimension sizes in [aclCreateTensorDesc](aclCreateTensorDesc.md)); the data type and Format settings are invalid.<br>The number of dimensions and dimension sizes set here must be within the input Shape range set during model compilation. |
| index | Input | Sequence number of the input or output.<br>When a model has multiple inputs and outputs, to avoid sequence number errors, call [aclmdlGetInputNameByIndex](aclmdlGetInputNameByIndex.md) and [aclmdlGetOutputNameByIndex](aclmdlGetOutputNameByIndex.md) first to obtain the input and output names, and then set the index based on the index corresponding to the input and output names. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).

## Constraints

For the same model, you can call only one of the following APIs: [aclmdlSetDynamicBatchSize](aclmdlSetDynamicBatchSize.md), [aclmdlSetDynamicHWSize](aclmdlSetDynamicHWSize.md), [aclmdlSetInputDynamicDims](aclmdlSetInputDynamicDims.md), or aclmdlSetDatasetTensorDesc.
