# aclSetTensorOriginShape

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T16:02:35.810Z pushedAt=2026-10-29T03:59:45.938Z nl=2 -->

## Product Support

<!-- npu="950" id477 -->
- Ascend 950PR&950DT products: Supported
<!-- end id477 -->
<!-- npu="A3" id478 -->
- Atlas A3 products: Supported
<!-- end id478 -->
<!-- npu="910b" id479 -->
- Atlas A2 products: Supported
<!-- end id479 -->
<!-- npu="310b" id480 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id480 -->
<!-- npu="310p" id481 -->
- Atlas inference products: Supported
<!-- end id481 -->
<!-- npu="910" id482 -->
- Atlas training products: Supported
<!-- end id482 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclSetTensorOriginShape_res.md#id1 -->

## Feature Description

After you call [aclCreateTensorDesc](aclCreateTensorDesc.md) to create a tensor description, you can call aclSetTensorOriginShape to set the original dims information of the tensor.

For example, the original Format of an operator may be NHWC (with a 4-dimensional original Shape) or NDHWC (with a 5-dimensional original Shape). To improve data access efficiency, the Format is automatically converted during model conversion or online network construction, transforming the original data Format into the Format used by the AI processor for internal computation (NC1HWC0 or NDC1HWC0), and the Shape is derived accordingly for internal computation.

## Prototype

```c
aclError aclSetTensorOriginShape(aclTensorDesc *desc, int numDims, const int64_t *dims)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| desc | Output | Pointer to the aclTensorDesc type.<br>Call the [aclCreateTensorDesc](aclCreateTensorDesc.md) API in advance to create the aclTensorDesc type. |
| numDims | Input | Number of dims dimensions to set. |
| dims | Input | Pointer to dims. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
