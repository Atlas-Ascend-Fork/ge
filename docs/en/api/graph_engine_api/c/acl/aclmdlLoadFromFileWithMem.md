# aclmdlLoadFromFileWithMem

<!-- md-trans-meta sourceCommit=aaa82213df16687539020ed5cae4f006363bb63c translatedAt=2026-11-17T14:35:33.746Z pushedAt=2026-11-18T03:34:52.609Z nl=2 -->

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
<!-- npu="IPV350" id308 -->
- IPV350: not supported
<!-- end id308 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlLoadFromFileWithMem_res.md#id1 -->

## Function Description

Loads offline model data from a file, with the memory for model execution managed by you.

The file passed through the modelPath parameter in this API is a \*.om or .om2 model file. For details about how to obtain an om or .om2 model file, see "[--mode](../../../../user_guides/atc_tools/CLI_options/--mode.md)" in *[ATC Offline Model Compilation Tool](../../../../user_guides/atc_tools/README.md)*.

## Prototype

```c
aclError aclmdlLoadFromFileWithMem(const char *modelPath, uint32_t *modelId, void *workPtr, size_t workSize, void *weightPtr, size_t weightSize)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| modelPath | Input | Pointer to the model file path, which includes the file name. The user running the application (APP) must have access permission to the storage path. |
| modelId | Output | Pointer to the model ID.<br>After the model is loaded successfully, the system returns the model ID, which identifies the model in subsequent operations. |
| workPtr | Input | Address pointer to the work memory required by the model on the Device. The work memory stores temporary data during model execution and is managed by you. It cannot be released during model execution.<br>If a null pointer is passed to workPtr, the system manages the memory.<br>When you manage the work memory, multiple models can share the same work memory if they are executed serially. However, you must ensure the serial execution order of the models and allocate the work memory based on the largest work memory size among the models. For example, you can ensure serial execution by locking during model execution to guarantee that execution tasks run serially. |
| workSize | Input | Size of the work memory required by the model, in bytes. This parameter is invalid when workPtr is a null pointer. |
| weightPtr | Input | Address pointer to the weight memory of the model on the Device. The weight memory stores weight data and is managed by you. It cannot be released during model execution.<br>If a null pointer is passed to weightPtr, the system manages the memory.<br>When you manage the weight memory, in a multi-thread scenario, if the same model is loaded once in each thread, the threads can share weightPtr because the weightPtr memory is read-only during inference. Note that weightPtr cannot be released while it is being shared. |
| weightSize | Input | Size of the weight memory required by the model, in bytes. This parameter is invalid when weightPtr is a null pointer. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).

## Reference Resources

The [aclmdlSetConfigOpt](aclmdlSetConfigOpt.md) and [aclmdlLoadWithConfig](aclmdlLoadWithConfig.md) APIs are also available for model loading. They use the properties in the configuration object to distinguish whether the model is loaded from a file or from memory, and whether the memory is managed internally by the system or by you.
