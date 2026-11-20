# aclSetTensorOriginShape

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-11-17T15:19:55.654Z pushedAt=2026-11-18T06:25:42.015Z nl=2 -->

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
<!-- npu="IPV350" id483 -->
- IPV350: not supported
<!-- end id483 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclSetTensorOriginShape_res.md#id1 -->

## Function Description

After you call [aclCreateTensorDesc](aclCreateTensorDesc.md) to create a tensor description, you can call aclSetTensorOriginShape to set the original dims information of the tensor.

For example, the original Format of an operator is NHWC (with a 4-dimensional original Shape) or NDHWC (with a 5-dimensional original Shape). To improve data access efficiency, the Format is automatically converted during model conversion or online network construction, transforming the original data Format into the Format used for internal computation on the AI processor (NC1HWC0 or NDC1HWC0), and the Shape is derived for internal computation at the same time.

## Prototype

```c
aclError aclSetTensorOriginShape(aclTensorDesc *desc, int numDims, const int64_t *dims)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| desc | Output | Pointer to the aclTensorDesc type.<br>Call [aclCreateTensorDesc](aclCreateTensorDesc.md) in advance to create the aclTensorDesc type. |
| numDims | Input | Number of dims dimensions to set. |
| dims | Input | Pointer to dims. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
