# aclmdlExecuteAsyncV2

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:06:46.226Z pushedAt=2026-10-29T03:06:22.815Z nl=2 -->

## Product Support

<!-- npu="950" id372 -->
- Ascend 950PR&950DT products: Not supported
<!-- end id372 -->
<!-- npu="A3" id373 -->
- Atlas A3 products: Not supported
<!-- end id373 -->
<!-- npu="910b" id374 -->
- Atlas A2 products: Not supported
<!-- end id374 -->
<!-- npu="310b" id375 -->
- Atlas 200I/500 A2 inference product: Not supported
<!-- end id375 -->
<!-- npu="310p" id376 -->
- Atlas inference products: Not supported
<!-- end id376 -->
<!-- npu="910" id377 -->
- Atlas training products: Not supported
<!-- end id377 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlExecuteAsyncV2_res.md#id1 -->

## Feature Description

Executes model inference based on the attributes configured by [aclmdlSetExecConfigOpt](aclmdlSetExecConfigOpt.md) until the inference result is returned. This API is an enhancement of [aclmdlExecuteAsync](aclmdlExecuteAsync.md) and supports controlling some configuration parameters during model inference. It is an asynchronous API.

This API must be used together with other APIs to implement model execution. The API call sequence is as follows:

1. Call [aclmdlCreateExecConfigHandle](aclmdlCreateExecConfigHandle.md) to create a configuration object for model execution.
2. Call [aclmdlSetExecConfigOpt](aclmdlSetExecConfigOpt.md) multiple times to set the value of each attribute in the configuration object.
3. Call aclmdlExecuteAsyncV2 to specify the configuration information required for model execution and perform model execution.
4. After model execution succeeds, call [aclmdlDestroyExecConfigHandle](aclmdlDestroyExecConfigHandle.md) to destroy the configuration object.

## Prototype

```c
aclError aclmdlExecuteAsyncV2(uint32_t modelId, const aclmdlDataset *input, aclmdlDataset *output, aclrtStream stream, const aclmdlExecConfigHandle *handle)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| modelId | Input | ID of the model to be executed for inference.<br>After the model is loaded successfully by calling [aclmdlLoadFromFile](aclmdlLoadFromFile.md)/[aclmdlLoadFromMem](aclmdlLoadFromMem.md)/[aclmdlLoadFromFileWithMem](aclmdlLoadFromFileWithMem.md)/[aclmdlLoadFromMemWithMem](aclmdlLoadFromMemWithMem.md), the model ID is returned. |
| input | Input | Pointer to the input data for model inference. See [aclmdlDataset](aclmdlDataset.md) for the type definition. |
| output | Output | Pointer to the output data for model inference. See [aclmdlDataset](aclmdlDataset.md) for the type definition. |
| stream | Input | Stream to be specified. See aclrtStream for the type definition. |
| handle | Input | Pointer to the configuration object for model execution. See [aclmdlExecConfigHandle](aclmdlExecConfigHandle.md) for the type definition.<br>Keep it consistent with the handle in [aclmdlSetExecConfigOpt](aclmdlSetExecConfigOpt.md). |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See also [aclError](aclError.md).

## Constraints

Other constraints are the same as those of [aclmdlExecuteAsync](aclmdlExecuteAsync.md).
