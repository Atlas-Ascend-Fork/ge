# aclmdlSetConfigOpt

<!-- md-trans-meta sourceCommit=aaa82213df16687539020ed5cae4f006363bb63c translatedAt=2026-11-17T14:50:21.375Z pushedAt=2026-11-18T03:38:25.780Z nl=2 -->

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
<!-- npu="IPV350" id887 -->
- IPV350: support
<!-- end id887 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlSetConfigOpt_res.md#id1 -->

## Function Description

Sets the value of each attribute in the configuration object for loading om or om2 models, including the model execution priority, the model file path or memory address, and the memory size.

**This API must be used together with the following APIs** to implement model loading:

1. Call the [aclmdlCreateConfigHandle](aclmdlCreateConfigHandle.md) API to create a configuration object for model loading.
2. (Optional) Call the [aclmdlSetExternalWeightAddress](aclmdlSetExternalWeightAddress.md) API to configure the Device memory for storing external weights.
3. Call the [aclmdlSetConfigOpt](aclmdlSetConfigOpt.md) API multiple times to set the value of each attribute in the configuration object.
4. Call the [aclmdlLoadWithConfig](aclmdlLoadWithConfig.md) API to specify the configuration information required for model loading and load the model.
5. After the model is loaded successfully, call the [aclmdlDestroyConfigHandle](aclmdlDestroyConfigHandle.md) API to destroy the configuration object.

## Prototype

```c
aclError aclmdlSetConfigOpt(aclmdlConfigHandle *handle, aclmdlConfigAttr attr, const void *attrValue, size_t valueSize)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| handle | Output | Pointer to the configuration object for model loading. For the type definition, see [aclmdlConfigHandle](aclmdlConfigHandle.md).<br>Call [aclmdlCreateConfigHandle](aclmdlCreateConfigHandle.md) in advance to create this object. |
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
