# aclmdlLoadWithConfig

<!-- md-trans-meta sourceCommit=aaa82213df16687539020ed5cae4f006363bb63c translatedAt=2026-11-17T14:39:00.170Z pushedAt=2026-11-18T03:35:24.613Z nl=2 -->

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
<!-- npu="IPV350" id434 -->
- IPV350: support
<!-- end id434 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlLoadWithConfig_res.md#id1 -->

## Function Description

Specifies the configuration information required for loading an om or om2 model and loads the model. Before loading, evaluate whether the memory space is sufficient based on the model file size. Insufficient memory space will cause application exceptions.

**This API must be used together with the following APIs** to implement model loading:

1. Call the [aclmdlCreateConfigHandle](aclmdlCreateConfigHandle.md) API to create a configuration object for model loading.
2. (Optional) Call the [aclmdlSetExternalWeightAddress](aclmdlSetExternalWeightAddress.md) API to configure the Device memory for storing external weights.
3. Call the [aclmdlSetConfigOpt](aclmdlSetConfigOpt.md) API multiple times to set the value of each attribute in the configuration object.
4. Call the [aclmdlLoadWithConfig](aclmdlLoadWithConfig.md) API to specify the configuration information required for model loading and load the model.
5. After the model is loaded successfully, call the [aclmdlDestroyConfigHandle](aclmdlDestroyConfigHandle.md) API to destroy the configuration object.

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

<!-- npu="IPV350" id1 -->
## Constraints

When loading a model from Device memory, do not modify or release the Device memory that stores the model data from the start of model loading until the model is unloaded by calling the aclmdlUnload API.
<!-- end id1 -->

## Reference Resources

When using the [aclmdlSetConfigOpt](aclmdlSetConfigOpt.md) API and the [aclmdlLoadWithConfig](aclmdlLoadWithConfig.md) API, the properties in the configuration object determine whether the model is loaded from a file or from memory, and whether the memory is managed internally by the system or by the user.

The following APIs are also provided to implement model loading. The API used determines whether the model is loaded from a file or from memory, and whether the memory is managed internally by the system or by the user.

- [aclmdlLoadFromFile](aclmdlLoadFromFile.md) API
- [aclmdlLoadFromMem](aclmdlLoadFromMem.md) API
- [aclmdlLoadFromFileWithMem](aclmdlLoadFromFileWithMem.md) API
- [aclmdlLoadFromMemWithMem](aclmdlLoadFromMemWithMem.md) API
- [aclmdlLoadFromFileWithQ](aclmdlLoadFromFileWithQ.md) API
- [aclmdlLoadFromMemWithQ](aclmdlLoadFromMemWithQ.md) API
