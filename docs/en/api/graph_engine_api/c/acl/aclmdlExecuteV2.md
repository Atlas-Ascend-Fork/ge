# aclmdlExecuteV2

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:06:52.882Z pushedAt=2026-10-29T03:06:34.126Z nl=2 -->

## Product Support

<!-- npu="950" id1140 -->
- Ascend 950PR&950DT products: Supported
<!-- end id1140 -->
<!-- npu="A3" id1141 -->
- Atlas A3 products: Supported
<!-- end id1141 -->
<!-- npu="910b" id1142 -->
- Atlas A2 products: Supported
<!-- end id1142 -->
<!-- npu="310b" id1143 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id1143 -->
<!-- npu="310p" id1144 -->
- Atlas inference products: Supported
<!-- end id1144 -->
<!-- npu="910" id1145 -->
- Atlas training products: Supported
<!-- end id1145 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlExecuteV2_res.md#id1 -->

## Feature Description

Executes model inference based on the attributes configured by the [aclmdlSetExecConfigOpt](aclmdlSetExecConfigOpt.md) API until the inference result is returned. This API is an enhanced version of [aclmdlExecute](aclmdlExecute.md), supporting control of some configuration parameters during model inference.

This API must be used together with other APIs to implement model execution. The API call sequence is as follows:

1. Call the [aclmdlCreateExecConfigHandle](aclmdlCreateExecConfigHandle.md) API to create a configuration object for model execution.
2. Call the [aclmdlSetExecConfigOpt](aclmdlSetExecConfigOpt.md) API multiple times to set the value of each attribute in the configuration object.
3. Call the aclmdlExecuteV2 API to specify the configuration information required for model execution and perform model execution.
4. After model execution succeeds, call the [aclmdlDestroyExecConfigHandle](aclmdlDestroyExecConfigHandle.md) API to destroy the configuration object.

## Prototype

```c
aclError aclmdlExecuteV2(uint32_t modelId, const aclmdlDataset *input, aclmdlDataset *output, aclrtStream stream, const aclmdlExecConfigHandle *handle)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| modelId | Input | ID of the model to be inferred.<br>After a model loading API (for example, [aclmdlLoadFromFile](aclmdlLoadFromFile.md) or [aclmdlLoadFromMem](aclmdlLoadFromMem.md)) is successfully called, a model ID is returned and used as the input of this API. |
| input | Input | Pointer to the input data for model inference. See [aclmdlDataset](aclmdlDataset.md) for the type definition. |
| output | Output | Pointer to the output data for model inference. See [aclmdlDataset](aclmdlDataset.md) for the type definition.<br>When calling aclCreateDataBuffer to create an aclDataBuffer object for storing the output data of the corresponding index, you can pass nullptr to the data parameter and set size to 0 to create an empty aclDataBuffer object. During model execution, the system internally calculates and allocates memory for the output of this index. This approach saves memory, but after the memory data is used, you must release the memory and reset the aclDataBuffer object. In addition, memory allocation by the system involves memory copy, which may cause performance loss. |
| stream | Input | Stream to be specified. See aclrtStream for the type definition. |
| handle | Input | Pointer to the configuration object for model execution. See [aclmdlExecConfigHandle](aclmdlExecConfigHandle.md) for the type definition.<br>Keep it consistent with the handle in [aclmdlSetExecConfigOpt](aclmdlSetExecConfigOpt.md). |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See also [aclError](aclError.md).

## Constraints

<!-- npu="950,A3,910b,910,310p,310b" id1 -->
If NULL is passed here, the attribute values configured through [aclmdlSetExecConfigOpt](aclmdlSetExecConfigOpt.md) do not take effect.
<!-- end id1 -->

Other constraints are the same as those of [aclmdlExecute](aclmdlExecute.md).

## Examples

The following sample code shows how to release memory and reset aclDataBuffer:

```c
aclDataBuffer *dataBuffer = aclmdlGetDatasetBuffer(output, 0); // Obtain the corresponding dataBuffer based on the index.
void *data = aclGetDataBufferAddr(dataBuffer);  // Obtain the device pointer of data.
aclrtFree(data ); // Release the device memory.
aclUpdateDataBuffer(dataBuffer, nullptr, 0); // Reset the content in dataBuffer for the next inference.
```
