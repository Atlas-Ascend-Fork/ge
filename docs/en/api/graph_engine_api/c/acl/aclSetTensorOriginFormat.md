# aclSetTensorOriginFormat

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T16:02:09.262Z pushedAt=2026-10-29T03:59:40.311Z nl=2 -->

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
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclSetTensorOriginFormat_res.md#id1 -->

## Feature Description

After creating a tensor description by calling [aclCreateTensorDesc](aclCreateTensorDesc.md), you can set the original Format information of the tensor through aclSetTensorOriginFormat.

For example, if the original Format of an operator is NHWC (with a 4-dimensional original Shape) or NDHWC (with a 5-dimensional original Shape), the Format is automatically converted during model conversion or online network construction to improve data access efficiency. The original data Format is converted to the Format used for internal computation on the AI processor (NC1HWC0 or NDC1HWC0), and the Shape is derived for internal computation.

## Prototype

```c
aclError aclSetTensorOriginFormat(aclTensorDesc *desc, aclFormat format)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| desc | Output | Pointer to the aclTensorDesc type.<br>Call the [aclCreateTensorDesc](aclCreateTensorDesc.md) API in advance to create the aclTensorDesc type. |
| format | Input | Format to set. See aclFormat for the type definition. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
