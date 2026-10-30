# aclmdlLoadFromMem

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:27:38.652Z pushedAt=2026-10-29T03:31:32.740Z nl=2 -->

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
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlLoadFromMem_res.md#id1 -->

## Feature Description

Loads the om model file data from memory. The system manages the memory used for model running.

For details about how to obtain the om model file, see "[--mode](../../../../user_guides/atc_tools/CLI_options/--mode.md)" in *[ATC Offline Model Compilation Tool](../../../../user_guides/atc_tools/README.md)*.

## Prototype

```c
aclError aclmdlLoadFromMem(const void *model, size_t modelSize, uint32_t *modelId)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| model | Input | Memory address pointer to the model data. |
| modelSize | Input | Length of the model data in memory, in bytes. |
| modelId | Output | Pointer to the model ID.<br>After the model is loaded successfully, the system returns the model ID, which is used to identify the model in subsequent operations. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).

## Constraints

<!-- npu="950,A3,910b,910,310p,310b" id1 -->
In Ascend EP form, memory for the model parameter must be allocated on the Host.
<!-- end id1 -->

<!-- npu="310b" id2 -->
In Ascend RC form, memory for the model parameter must be allocated on the Device.
<!-- end id2 -->

<!-- npu="310p" id3 -->
In Control CPU open form, memory for the model parameter must be allocated on the Device.
<!-- end id3 -->

<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlLoadFromMem_res.md#id2 -->

## References

The [aclmdlSetConfigOpt](aclmdlSetConfigOpt.md) API and the [aclmdlLoadWithConfig](aclmdlLoadWithConfig.md) API are also available for model loading. They use the attributes in the configuration object to distinguish whether the model is loaded from a file or from memory, and whether the memory is managed internally by the system or by you.
