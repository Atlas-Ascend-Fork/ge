# aclmdlSetExternalWeightAddress

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:40:44.629Z pushedAt=2026-10-29T03:40:28.708Z nl=2 -->

## Product Support

<!-- npu="950" id57 -->
- Ascend 950PR&950DT products: Supported
<!-- end id57 -->
<!-- npu="A3" id58 -->
- Atlas A3 products: Supported
<!-- end id58 -->
<!-- npu="910b" id59 -->
- Atlas A2 products: Supported
<!-- end id59 -->
<!-- npu="310b" id60 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id60 -->
<!-- npu="310p" id61 -->
- Atlas inference products: Supported
<!-- end id61 -->
<!-- npu="910" id62 -->
- Atlas training products: Supported
<!-- end id62 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlSetExternalWeightAddress_res.md#id1 -->

## Feature Description

Configures the Device memory for storing external weights. The caller must copy the external weight data to this memory before model loading. If there are multiple external weight files, call this API multiple times.

During model loading, each FileConstant node in the compute graph stores the path of the corresponding external weight file (for example, /xx/../weight\_hasid1). The Graph Engine (GE) checks, based on the file name in the path (for example, weight\_hasid1), whether you have configured Device memory for the external weights. If configured, GE directly uses the Device memory address; otherwise, GE allocates a block of Device memory, reads the external weight file, and copies the external weight data to the Device memory. This memory is released when the model is unloaded.

![](figures/Device-Context-Stream-relationship.png)

**This API must be used together with the following APIs** to implement model loading:

1. Call the [aclmdlCreateConfigHandle](aclmdlCreateConfigHandle.md) API to create a configuration object for model loading.
2. (Optional) Call the [aclmdlSetExternalWeightAddress](aclmdlSetExternalWeightAddress.md) API to configure the Device memory for storing external weights.
3. Call the [aclmdlSetConfigOpt](aclmdlSetConfigOpt.md) API multiple times to set the value of each attribute in the configuration object.
4. Call the [aclmdlLoadWithConfig](aclmdlLoadWithConfig.md) API to specify the configuration information required for model loading and load the model.
5. After the model is loaded successfully, call the [aclmdlDestroyConfigHandle](aclmdlDestroyConfigHandle.md) API to destroy the configuration object.

## Prototype

```c
aclError aclmdlSetExternalWeightAddress(aclmdlConfigHandle *handle, const char *weightFileName, void *devPtr, size_t size)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| handle | Output | Pointer to the configuration object for model loading. For the type definition, see [aclmdlConfigHandle](aclmdlConfigHandle.md).<br>Call [aclmdlCreateConfigHandle](aclmdlCreateConfigHandle.md) in advance to create this object. |
| weightFileName | Input | Name of the external weight file, excluding the path.<br>Generally, when the om model file size is limited or the model file is encrypted, the weight file needs to be loaded separately. Therefore, when building the model, save the weights in a separate file. For example, when using the ATC tool to generate an om file, set the --external_weight parameter to 1 (1 means saving the weights of the Const/Constant nodes in the original network to a separate file). |
| devPtr | Input | Pointer to the memory address on the Device where the external weights are stored.<br>This Device memory is managed by you and cannot be released during model execution. |
| size | Input | Memory size, in bytes, which must be 32-byte aligned. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
