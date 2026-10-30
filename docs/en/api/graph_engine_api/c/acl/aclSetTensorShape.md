# aclSetTensorShape

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T16:03:29.050Z pushedAt=2026-10-29T03:59:56.309Z nl=2 -->

## Product Support

<!-- npu="950" id762 -->
- Ascend 950PR&950DT products: Supported
<!-- end id762 -->
<!-- npu="A3" id763 -->
- Atlas A3 products: Supported
<!-- end id763 -->
<!-- npu="910b" id764 -->
- Atlas A2 products: Supported
<!-- end id764 -->
<!-- npu="310b" id765 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id765 -->
<!-- npu="310p" id766 -->
- Atlas inference products: Supported
<!-- end id766 -->
<!-- npu="910" id767 -->
- Atlas training products: Not supported
<!-- end id767 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclSetTensorShape_res.md#id1 -->

## Feature Description

After you call [aclCreateTensorDesc](aclCreateTensorDesc.md) to create the tensor description, you can call aclSetTensorShape to set the dims information used when the AI processor processes the tensor internally.

For example, the original Format of an operator is NHWC (with a 4-dimensional original Shape) or NDHWC (with a 5-dimensional original Shape). To improve data access efficiency, the Format is automatically converted during model conversion or online network construction, transforming the original data Format into the Format used for internal computation on the AI processor (NC1HWC0 or NDC1HWC0), and the Shape is derived for internal computation at the same time.

## Prototype

```c
aclError aclSetTensorShape(aclTensorDesc *desc, int numDims, const int64_t *dims)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| desc | Output | Pointer to the aclTensorDesc type.<br>Call [aclCreateTensorDesc](aclCreateTensorDesc.md) in advance to create the aclTensorDesc type. |
| numDims | Input | Number of dims dimensions to set. |
| dims | Input | Pointer to dims. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
