# aclmdlLoadWithConfig

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:29:43.147Z pushedAt=2026-10-29T03:32:00.708Z nl=2 -->

## Product Support

<!-- npu="950" id428 -->
- Ascend 950PR&950DT products: Supported
<!-- end id428 -->
<!-- npu="A3" id429 -->
- Atlas A3 products: Supported
<!-- end id429 -->
<!-- npu="910b" id430 -->
- Atlas A2 products: Supported
<!-- end id430 -->
<!-- npu="310b" id431 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id431 -->
<!-- npu="310p" id432 -->
- Atlas inference products: Supported
<!-- end id432 -->
<!-- npu="910" id433 -->
- Atlas training products: Supported
<!-- end id433 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlLoadWithConfig_res.md#id1 -->

## Feature Description

Specifies the configuration information required for model loading and loads the model. Before loading, evaluate whether the memory space is sufficient based on the model file size. Insufficient memory space will cause application exceptions.

**This API must be used with the following APIs** to implement model loading:

1. Call [aclmdlCreateConfigHandle](aclmdlCreateConfigHandle.md) to create a configuration object for model loading.
2. (Optional) Call [aclmdlSetExternalWeightAddress](aclmdlSetExternalWeightAddress.md) to configure the Device memory for storing external weights.
3. Call [aclmdlSetConfigOpt](aclmdlSetConfigOpt.md) multiple times to set the value of each attribute in the configuration object.
4. Call [aclmdlLoadWithConfig](aclmdlLoadWithConfig.md) to specify the configuration information required for model loading and load the model.
5. After the model is loaded successfully, call [aclmdlDestroyConfigHandle](aclmdlDestroyConfigHandle.md) to destroy the configuration object.

## Prototype

```c
aclError aclmdlLoadWithConfig(const aclmdlConfigHandle *handle, uint32_t *modelId)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| handle | Input | Pointer to the configuration object for model loading. See [aclmdlConfigHandle](aclmdlConfigHandle.md) for the type definition.<br>Call [aclmdlCreateConfigHandle](aclmdlCreateConfigHandle.md) in advance to create this object, and keep it consistent with the handle in aclmdlSetConfigOpt. |
| modelId | Output | Pointer to the model ID.<br>Model ID returned after the model is successfully loaded. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).

## References

When using the [aclmdlSetConfigOpt](aclmdlSetConfigOpt.md) API and the [aclmdlLoadWithConfig](aclmdlLoadWithConfig.md) API, the properties in the configuration object determine whether the model is loaded from a file or from memory, and whether the memory is managed internally by the system or by the user.

The following APIs are also provided for model loading. The API used determines whether the model is loaded from a file or from memory, and whether the memory is managed internally by the system or by the user.

- [aclmdlLoadFromFile](aclmdlLoadFromFile.md) API
- [aclmdlLoadFromMem](aclmdlLoadFromMem.md) API
- [aclmdlLoadFromFileWithMem](aclmdlLoadFromFileWithMem.md) API
- [aclmdlLoadFromMemWithMem](aclmdlLoadFromMemWithMem.md) API
- [aclmdlLoadFromFileWithQ](aclmdlLoadFromFileWithQ.md) API
- [aclmdlLoadFromMemWithQ](aclmdlLoadFromMemWithQ.md) API
