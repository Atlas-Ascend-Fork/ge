# aclSetTensorFormat

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-11-17T15:19:08.026Z pushedAt=2026-11-18T06:16:26.140Z nl=2 -->

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
<!-- npu="IPV350" id684 -->
- IPV350: not supported
<!-- end id684 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclSetTensorFormat_res.md#id1 -->

## Function Description

After you call [aclCreateTensorDesc](aclCreateTensorDesc.md) to create a tensor description, you can call aclSetTensorFormat to set the Format used by the AI processor when processing the tensor internally.

For example, the original Format of an operator is NHWC (with a 4-dimensional original Shape) or NDHWC (with a 5-dimensional original Shape). To improve data access efficiency, the Format is automatically converted during model conversion or online network construction, transforming the original data Format into the Format used for internal computation on the AI processor (NC1HWC0 or NDC1HWC0), and the Shape is derived for internal computation at the same time.

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
