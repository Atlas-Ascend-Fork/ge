# aclSetTensorOriginFormat

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-11-17T15:19:09.277Z pushedAt=2026-11-18T06:16:36.462Z nl=2 -->

## Product Support

<!-- npu="950" id853 -->
- Ascend 950PR&950DT products: Supported
<!-- end id853 -->
<!-- npu="A3" id854 -->
- Atlas A3 products: Supported
<!-- end id854 -->
<!-- npu="910b" id855 -->
- Atlas A2 products: Supported
<!-- end id855 -->
<!-- npu="310b" id856 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id856 -->
<!-- npu="310p" id857 -->
- Atlas inference products: Supported
<!-- end id857 -->
<!-- npu="910" id858 -->
- Atlas training products: Supported
<!-- end id858 -->
<!-- npu="IPV350" id859 -->
- IPV350: not supported
<!-- end id859 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclSetTensorOriginFormat_res.md#id1 -->

## Function Description

After creating a tensor description by calling [aclCreateTensorDesc](aclCreateTensorDesc.md), you can set the original Format information of the tensor through aclSetTensorOriginFormat.

For example, the original Format of an operator is NHWC (with a 4-dimensional original Shape) or NDHWC (with a 5-dimensional original Shape). To improve data access efficiency, the Format is automatically converted during model conversion or online network construction, transforming the original data Format into the Format used for internal computation on the AI processor (NC1HWC0 or NDC1HWC0), and the Shape is derived for internal computation at the same time.

## Prototype

```c
aclError aclSetTensorOriginFormat(aclTensorDesc *desc, aclFormat format)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| desc | Output | Pointer to the aclTensorDesc type.<br>Call [aclCreateTensorDesc](aclCreateTensorDesc.md) in advance to create the aclTensorDesc type. |
| format | Input | Format to set. See aclFormat for the type definition. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
