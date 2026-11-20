# aclmdlLoadFromMemWithMem

<!-- md-trans-meta sourceCommit=aaa82213df16687539020ed5cae4f006363bb63c translatedAt=2026-11-17T14:37:10.301Z pushedAt=2026-11-18T03:35:07.777Z nl=2 -->

## Product Support

<!-- npu="950" id699 -->
- Ascend 950PR&950DT products: Supported
<!-- end id699 -->
<!-- npu="A3" id700 -->
- Atlas A3 products: Supported
<!-- end id700 -->
<!-- npu="910b" id701 -->
- Atlas A2 products: Supported
<!-- end id701 -->
<!-- npu="310b" id702 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id702 -->
<!-- npu="310p" id703 -->
- Atlas inference products: Supported
<!-- end id703 -->
<!-- npu="910" id704 -->
- Atlas training products: Supported
<!-- end id704 -->
<!-- npu="IPV350" id705 -->
- IPV350: not supported
<!-- end id705 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlLoadFromMemWithMem_res.md#id1 -->

## Function Description

Loads the om or .om2 model file data from memory, with the memory for model execution managed by the user.

For details about how to obtain an om or .om2 model file, see "[--mode](../../../../user_guides/atc_tools/CLI_options/--mode.md)" in *[ATC Offline Model Compilation Tool](../../../../user_guides/atc_tools/README.md)*.

## Prototype

```c
aclError aclmdlLoadFromMemWithMem(const void *model, size_t modelSize, uint32_t *modelId, void *workPtr, size_t workSize, void *weightPtr, size_t weightSize)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| model | Input | Memory address pointer to the model data. |
| modelSize | Input | Length of the model data, in bytes. |
| modelId | Output | Pointer to the model ID.<br>After the model is loaded successfully, the system returns the model ID, which identifies the model in subsequent operations. |
| workPtr | Input | Address pointer to the work memory required by the model on the Device. The work memory stores temporary data during model execution and is managed by you. It cannot be released during model execution.<br>If a null pointer is passed to workPtr, the system manages the memory.<br>When you manage the work memory, multiple models can share the same work memory if they are executed serially. However, you must ensure the serial execution order of the models and allocate the work memory based on the largest work memory size among the models. For example, you can ensure serial execution by locking during model execution to guarantee that execution tasks run serially. |
| workSize | Input | Size of the work memory required by the model, in bytes. This parameter is invalid when workPtr is a null pointer. |
| weightPtr | Input | Address pointer to the weight memory of the model on the Device. The weight memory stores weight data and is managed by you. It cannot be released during model execution.<br>If a null pointer is passed to weightPtr, the system manages the memory.<br>When you manage the weight memory, in a multi-thread scenario, if the same model is loaded once in each thread, the threads can share weightPtr because the weightPtr memory is read-only during inference. Note that weightPtr cannot be released while it is being shared. |
| weightSize | Input | Size of the weight memory required by the model, in bytes. This parameter is invalid when weightPtr is a null pointer. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).

## Constraints

<!-- npu="950,A3,910b,910,310p,310b" id1 -->
In Ascend EP mode, allocate memory on the Host for the model parameter.
<!-- end id1 -->

<!-- npu="310b" id2 -->
In Ascend RC mode, allocate memory on the Device for the model parameter.
<!-- end id2 -->

<!-- npu="310p" id3 -->
In Control CPU open mode, allocate memory on the Device for the model parameter.
<!-- end id3 -->

<!-- npu="IPV350" id4 -->
Memory must be allocated for the model parameter on the Device.
<!-- end id4 -->

<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlLoadFromMemWithMem_res.md#id2 -->

## Reference Resources

The [aclmdlSetConfigOpt](aclmdlSetConfigOpt.md) and [aclmdlLoadWithConfig](aclmdlLoadWithConfig.md) APIs are also available for model loading. They use the properties in the configuration object to distinguish whether the model is loaded from a file or from memory, and whether the memory is managed internally by the system or by you.
