# aclmdlLoadFromMem

<!-- md-trans-meta sourceCommit=aaa82213df16687539020ed5cae4f006363bb63c translatedAt=2026-11-17T14:36:01.767Z pushedAt=2026-11-18T03:35:02.220Z nl=2 -->

## Product Support

<!-- npu="950" id421 -->
- Ascend 950PR&950DT products: Supported
<!-- end id421 -->
<!-- npu="A3" id422 -->
- Atlas A3 products: Supported
<!-- end id422 -->
<!-- npu="910b" id423 -->
- Atlas A2 products: Supported
<!-- end id423 -->
<!-- npu="310b" id424 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id424 -->
<!-- npu="310p" id425 -->
- Atlas inference products: Supported
<!-- end id425 -->
<!-- npu="910" id426 -->
- Atlas training products: Supported
<!-- end id426 -->
<!-- npu="IPV350" id427 -->
- IPV350: not supported
<!-- end id427 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlLoadFromMem_res.md#id1 -->

## Function Description

Loads the om or .om2 model file data from memory. The system manages the memory used for model running.

For details about how to obtain an om or .om2 model file, see "[--mode](../../../../user_guides/atc_tools/CLI_options/--mode.md)" in *[ATC Offline Model Compilation Tool](../../../../user_guides/atc_tools/README.md)*.

## Prototype

```c
aclError aclmdlLoadFromMem(const void *model, size_t modelSize, uint32_t *modelId)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| model | Input | Memory address pointer to the model data. |
| modelSize | Input | Length of the model data in memory, in bytes. |
| modelId | Output | Pointer to the model ID.<br>After the model is loaded successfully, the system returns the model ID, which identifies the model in subsequent operations. |

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
At the model parameter, memory must be allocated on the Device.
<!-- end id4 -->

<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlLoadFromMem_res.md#id2 -->

## Reference Resources

The [aclmdlSetConfigOpt](aclmdlSetConfigOpt.md) and [aclmdlLoadWithConfig](aclmdlLoadWithConfig.md) APIs are also available for model loading. They use the properties in the configuration object to distinguish whether the model is loaded from a file or from memory, and whether the memory is managed internally by the system or by you.
