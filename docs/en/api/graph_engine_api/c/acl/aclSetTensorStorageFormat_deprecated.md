# aclSetTensorStorageFormat (Deprecated)

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T16:04:24.086Z pushedAt=2026-10-29T04:00:21.572Z nl=2 -->

**Notice: This API will be deprecated in later versions. Use [aclSetTensorFormat](aclSetTensorFormat.md) instead.**

## Product Support

<!-- npu="950" id553 -->
- Ascend 950PR&950DT products: Supported
<!-- end id553 -->
<!-- npu="A3" id554 -->
- Atlas A3 products: Supported
<!-- end id554 -->
<!-- npu="910b" id555 -->
- Atlas A2 products: Supported
<!-- end id555 -->
<!-- npu="310b" id556 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id556 -->
<!-- npu="310p" id557 -->
- Atlas inference products: Supported
<!-- end id557 -->
<!-- npu="910" id558 -->
- Atlas training products: Not supported
<!-- end id558 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclSetTensorStorageFormat_deprecated_res.md#id1 -->

## Feature Description

After you create the tensor description by calling [aclCreateTensorDesc](aclCreateTensorDesc.md), you can use aclSetTensorStorageFormat to set the actual Format information of the tensor.

## Prototype

```c
aclError aclSetTensorStorageFormat(aclTensorDesc *desc, aclFormat format)
```

## Parameters

| parameter name | Input/Output | Description |
| --- | --- | --- |
| desc | Output | Pointer to the aclTensorDesc type.<br>Call the [aclCreateTensorDesc](aclCreateTensorDesc.md) API in advance to create the aclTensorDesc type. |
| format | Input | Format to set. See aclFormat for the type definition. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
