# aclmdlLoadFromFileWithMem

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:27:22.241Z pushedAt=2026-10-29T03:31:13.967Z nl=2 -->

## Product Support

<!-- npu="950" id302 -->
- Ascend 950PR&950DT products: Supported
<!-- end id302 -->
<!-- npu="A3" id303 -->
- Atlas A3 products: Supported
<!-- end id303 -->
<!-- npu="910b" id304 -->
- Atlas A2 products: Supported
<!-- end id304 -->
<!-- npu="310b" id305 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id305 -->
<!-- npu="310p" id306 -->
- Atlas inference products: Supported
<!-- end id306 -->
<!-- npu="910" id307 -->
- Atlas training products: Supported
<!-- end id307 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlLoadFromFileWithMem_res.md#id1 -->

## Feature Description

Loads offline model data from a file, with the memory for model execution managed by you.

The file passed through the modelPath parameter in this API is a \*.om model file. For details about how to obtain an om model file, see "[--mode](../../../../user_guides/atc_tools/CLI_options/--mode.md)" in *[ATC Offline Model Compilation Tool](../../../../user_guides/atc_tools/README.md)*.

## Prototype

```c
aclError aclmdlLoadFromFileWithMem(const char *modelPath, uint32_t *modelId, void *workPtr, size_t workSize, void *weightPtr, size_t weightSize)
```

## Parameters

| Parameter Name | Input/Output | Description |
| --- | --- | --- |
| modelPath | Input | Pointer to the model file path, including the file name. The user running the application (APP) must have access permission to this storage path. |
| modelId | Output | Pointer to the model ID.<br>After the model is loaded successfully, the system returns the model ID, which serves as the identifier for recognizing the model in subsequent operations. |
| workPtr | Input | Address pointer to the working memory required by the model on the Device, which stores temporary data during model execution. This memory is managed by you and cannot be released during model execution.<br>If a null pointer is passed to workPtr, the system manages the memory.<br>When you manage the working memory, if multiple models are executed serially, they can share the same working memory. However, you must ensure the serial execution order of the models, and the working memory size must be allocated based on the largest working memory among the multiple models. For example, ensure serial execution by locking during model execution to guarantee that execution tasks run serially. |
| workSize | Input | Size of the working memory required by the model, in bytes. This parameter is invalid when workPtr is a null pointer. |
| weightPtr | Input | Address pointer to the model weight memory on the Device, which stores weight data. This memory is managed by you and cannot be released during model execution.<br>If a null pointer is passed to weightPtr, the system manages the memory.<br>When you manage the weight memory, in a multi-threaded scenario, if the same model is loaded once in each thread, weightPtr can be shared because the weightPtr memory is read-only during inference. Note that weightPtr cannot be released while it is being shared. |
| weightSize | Input | Size of the weight memory required by the model, in bytes. This parameter is invalid when weightPtr is a null pointer. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).

## Related Resources

The [aclmdlSetConfigOpt](aclmdlSetConfigOpt.md) API and the [aclmdlLoadWithConfig](aclmdlLoadWithConfig.md) API are also provided for model loading. They are distinguished by the properties in the configuration object, which determine whether the model is loaded from a file or from memory, and whether the memory is managed by the system or by you.
