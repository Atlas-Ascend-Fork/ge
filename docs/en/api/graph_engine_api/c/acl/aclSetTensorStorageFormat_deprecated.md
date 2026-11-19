# aclSetTensorStorageFormat (Deprecated)

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-11-17T15:21:51.997Z pushedAt=2026-11-18T06:27:35.798Z nl=2 -->

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
<!-- npu="IPV350" id559 -->
- IPV350: not supported
<!-- end id559 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclSetTensorStorageFormat_deprecated_res.md#id1 -->

## Function Description

After you create the tensor description by calling [aclCreateTensorDesc](aclCreateTensorDesc.md), you can use aclSetTensorStorageFormat to set the actual Format information of the tensor.

## Prototype

```c
aclError aclSetTensorStorageFormat(aclTensorDesc *desc, aclFormat format)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| desc | Output | Pointer to the aclTensorDesc type.<br>Call [aclCreateTensorDesc](aclCreateTensorDesc.md) in advance to create the aclTensorDesc type. |
| format | Input | Format to set. See aclFormat for the type definition. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
