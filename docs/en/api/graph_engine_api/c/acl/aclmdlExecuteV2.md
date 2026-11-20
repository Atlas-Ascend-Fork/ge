# aclmdlExecuteV2

<!-- md-trans-meta sourceCommit=aaa82213df16687539020ed5cae4f006363bb63c translatedAt=2026-11-17T14:13:40.908Z pushedAt=2026-11-18T03:29:21.445Z nl=2 -->

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
<!-- npu="IPV350" id1146 -->
- IPV350: support
<!-- end id1146 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlExecuteV2_res.md#id1 -->

## Function Description

Executes om or om2 model inference based on the attributes configured by the [aclmdlSetExecConfigOpt](aclmdlSetExecConfigOpt.md) API until the inference result is returned. This API is an enhanced version of [aclmdlExecute](aclmdlExecute.md), supporting control of some configuration parameters during model inference.

This API must be used together with other APIs to implement model execution. The API call sequence is as follows:

1. Call [aclmdlCreateExecConfigHandle](aclmdlCreateExecConfigHandle.md) to create a model execution configuration object.
2. Call [aclmdlSetExecConfigOpt](aclmdlSetExecConfigOpt.md) multiple times to set the value of each attribute in the configuration object.
3. Call the aclmdlExecuteV2 API to specify the configuration information required for model execution and perform model execution.
4. After model execution succeeds, call [aclmdlDestroyExecConfigHandle](aclmdlDestroyExecConfigHandle.md) to destroy the configuration object.

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

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).

## Constraints

<!-- npu="950,A3,910b,910,310p,310b" id1 -->
If NULL is passed here, the attribute values configured through [aclmdlSetExecConfigOpt](aclmdlSetExecConfigOpt.md) do not take effect.
<!-- end id1 -->

<!-- npu="IPV350" id2 -->
Passing NULL is not supported; otherwise, an error is returned.
<!-- end id2 -->

Other constraints are the same as those of [aclmdlExecute](aclmdlExecute.md).

## Example Code

The sample code for freeing memory and resetting aclDataBuffer is as follows:

```c
aclDataBuffer *dataBuffer = aclmdlGetDatasetBuffer(output, 0); // Obtain the corresponding dataBuffer by index.
void *data = aclGetDataBufferAddr(dataBuffer);  // Obtain the Device pointer of data.
aclrtFree(data ); // Free the Device memory.
aclUpdateDataBuffer(dataBuffer, nullptr, 0); // Reset the content in dataBuffer for the next inference.
```
