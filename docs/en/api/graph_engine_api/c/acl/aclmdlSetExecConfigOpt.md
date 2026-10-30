# aclmdlSetExecConfigOpt

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:40:13.443Z pushedAt=2026-10-29T03:39:30.101Z nl=2 -->

## Product Support

<!-- npu="950" id232 -->
- Ascend 950PR&950DT products: Supported
<!-- end id232 -->
<!-- npu="A3" id233 -->
- Atlas A3 products: Supported
<!-- end id233 -->
<!-- npu="910b" id234 -->
- Atlas A2 products: Supported
<!-- end id234 -->
<!-- npu="310b" id235 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id235 -->
<!-- npu="310p" id236 -->
- Atlas inference products: Supported
<!-- end id236 -->
<!-- npu="910" id237 -->
- Atlas training products: Supported
<!-- end id237 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlSetExecConfigOpt_res.md#id1 -->

## Feature Description

Sets the value of each attribute in the model execution configuration object.

This API must be used together with other APIs to implement model execution. The API call sequence is as follows:

1. Call [aclmdlCreateExecConfigHandle](aclmdlCreateExecConfigHandle.md) to create a model execution configuration object.
2. Call [aclmdlSetExecConfigOpt](aclmdlSetExecConfigOpt.md) multiple times to set the value of each attribute in the configuration object.
3. Call [aclmdlExecuteV2](aclmdlExecuteV2.md) or [aclmdlExecuteAsyncV2](aclmdlExecuteAsyncV2.md) to specify the configuration information required for model execution and perform model execution.
4. After model execution succeeds, call [aclmdlDestroyExecConfigHandle](aclmdlDestroyExecConfigHandle.md) to destroy the configuration object.

## Prototype

```c
aclError aclmdlSetExecConfigOpt(aclmdlExecConfigHandle *handle, aclmdlExecConfigAttr attr, const void *attrValue, size_t valueSize)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| handle | Output | Pointer to the configuration object for model execution. See [aclmdlExecConfigHandle](aclmdlExecConfigHandle.md) for the type definition.<br>Call [aclmdlCreateExecConfigHandle](aclmdlCreateExecConfigHandle.md) in advance to create this object. |
| attr | Input | Attribute to be set. See [aclmdlExecConfigAttr](aclmdlExecConfigAttr.md) for the type definition. |
| attrValue | Input | Pointer to the attribute value, which is the value corresponding to attr.<br>If the attribute value itself is a pointer, pass the address of the pointer. |
| valueSize | Input | Data length of attrValue.<br>You can use the C/C++ standard library function sizeof(*attrValue) to query the data length. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
