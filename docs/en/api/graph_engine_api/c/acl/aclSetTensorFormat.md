# aclSetTensorFormat

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T16:01:47.543Z pushedAt=2026-10-29T03:59:34.176Z nl=2 -->

## Product Support

<!-- npu="950" id678 -->
- Ascend 950PR&950DT products: Supported
<!-- end id678 -->
<!-- npu="A3" id679 -->
- Atlas A3 products: Supported
<!-- end id679 -->
<!-- npu="910b" id680 -->
- Atlas A2 products: Supported
<!-- end id680 -->
<!-- npu="310b" id681 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id681 -->
<!-- npu="310p" id682 -->
- Atlas inference products: Supported
<!-- end id682 -->
<!-- npu="910" id683 -->
- Atlas training products: Not supported
<!-- end id683 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclSetTensorFormat_res.md#id1 -->

## Feature Description

After you call [aclCreateTensorDesc](aclCreateTensorDesc.md) to create a tensor description, you can call aclSetTensorFormat to set the Format used by the AI processor when processing the tensor internally.

For example, if the original Format of an operator is NHWC (with a 4-dimensional original Shape) or NDHWC (with a 5-dimensional original Shape), the Format is automatically converted during model conversion or online network construction to improve data access efficiency. The original data Format is converted to the Format used by the AI processor for internal computation (NC1HWC0 or NDC1HWC0), and the Shape is derived for internal computation at the same time.

## Prototype

```c
aclError aclSetTensorFormat(aclTensorDesc *desc, aclFormat format)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| desc | Output | Pointer to the aclTensorDesc type.<br>Call [aclCreateTensorDesc](aclCreateTensorDesc.md) in advance to create the aclTensorDesc type. |
| format | Input | Format to set. See aclFormat for the type definition. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
