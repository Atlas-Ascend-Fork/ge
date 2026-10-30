# aclmdlSetConfigOpt

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:37:52.982Z pushedAt=2026-10-29T03:38:22.454Z nl=2 -->

## Product Support

<!-- npu="950" id881 -->
- Ascend 950PR&950DT products: Supported
<!-- end id881 -->
<!-- npu="A3" id882 -->
- Atlas A3 products: Supported
<!-- end id882 -->
<!-- npu="910b" id883 -->
- Atlas A2 products: Supported
<!-- end id883 -->
<!-- npu="310b" id884 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id884 -->
<!-- npu="310p" id885 -->
- Atlas inference products: Supported
<!-- end id885 -->
<!-- npu="910" id886 -->
- Atlas training products: Supported
<!-- end id886 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlSetConfigOpt_res.md#id1 -->

## Feature Description

Sets the value of each attribute in the configuration object for model loading, including the model execution priority, the model file path or memory address, and the memory size.

**This API must be used together with the following APIs** to implement model loading:

1. Call [aclmdlCreateConfigHandle](aclmdlCreateConfigHandle.md) to create the configuration object for model loading.
2. (Optional) Call [aclmdlSetExternalWeightAddress](aclmdlSetExternalWeightAddress.md) to configure the device memory for storing external weights.
3. Call [aclmdlSetConfigOpt](aclmdlSetConfigOpt.md) multiple times to set the value of each attribute in the configuration object.
4. Call [aclmdlLoadWithConfig](aclmdlLoadWithConfig.md) to specify the configuration information required for model loading and load the model.
5. After the model is loaded successfully, call [aclmdlDestroyConfigHandle](aclmdlDestroyConfigHandle.md) to destroy the configuration object.

## Prototype

```c
aclError aclmdlSetConfigOpt(aclmdlConfigHandle *handle, aclmdlConfigAttr attr, const void *attrValue, size_t valueSize)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| handle | Output | Pointer to the configuration object for model loading. See [aclmdlConfigHandle](aclmdlConfigHandle.md) for the type definition.<br>Call [aclmdlCreateConfigHandle](aclmdlCreateConfigHandle.md) in advance to create this object. |
| attr | Input | Attribute to be set. See [aclmdlConfigAttr](aclmdlConfigAttr.md) for the type definition. |
| attrValue | Input | Pointer to the attribute value, which is the value corresponding to attr.<br>If the attribute value itself is a pointer, pass the address of the pointer. |
| valueSize | Input | Data length of attrValue.<br>You can use the C/C++ standard library function sizeof(*attrValue) to query the data length. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).

## Reference Resources

When using the [aclmdlSetConfigOpt](aclmdlSetConfigOpt.md) API and the [aclmdlLoadWithConfig](aclmdlLoadWithConfig.md) API, the properties in the configuration object determine whether the model is loaded from a file or from memory, and whether the memory is managed internally by the system or by the user.

The following APIs are also provided to implement model loading. The API used determines whether the model is loaded from a file or from memory, and whether the memory is managed internally by the system or by the user.

- [aclmdlLoadFromFile](aclmdlLoadFromFile.md) API
- [aclmdlLoadFromMem](aclmdlLoadFromMem.md) API
- [aclmdlLoadFromFileWithMem](aclmdlLoadFromFileWithMem.md) API
- [aclmdlLoadFromMemWithMem](aclmdlLoadFromMemWithMem.md) API
- [aclmdlLoadFromFileWithQ](aclmdlLoadFromFileWithQ.md) API
- [aclmdlLoadFromMemWithQ](aclmdlLoadFromMemWithQ.md) API
