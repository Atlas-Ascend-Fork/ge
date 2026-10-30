# aclSetTensorStorageShape (Deprecated)

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T16:04:54.356Z pushedAt=2026-10-29T04:00:28.202Z nl=2 -->

**Note: This API will be deprecated in later versions. Use the [aclSetTensorShape](aclSetTensorShape.md) API instead.**

## Product Support

<!-- npu="950" id539 -->
- Ascend 950PR&950DT products: Supported
<!-- end id539 -->
<!-- npu="A3" id540 -->
- Atlas A3 products: Supported
<!-- end id540 -->
<!-- npu="910b" id541 -->
- Atlas A2 products: Supported
<!-- end id541 -->
<!-- npu="310b" id542 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id542 -->
<!-- npu="310p" id543 -->
- Atlas inference products: Supported
<!-- end id543 -->
<!-- npu="910" id544 -->
- Atlas training products: Not supported
<!-- end id544 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclSetTensorStorageShape_deprecated_res.md#id1 -->

## Feature Description

After creating the tensor description by calling the [aclCreateTensorDesc](aclCreateTensorDesc.md) API, you can set the actual dims information of the tensor through the aclSetTensorStorageShape API.

## Prototype

```c
aclError aclSetTensorStorageShape(aclTensorDesc *desc, int numDims, const int64_t *dims)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| desc | Output | Pointer to the aclTensorDesc type.<br>Call [aclCreateTensorDesc](aclCreateTensorDesc.md) in advance to create the aclTensorDesc type. |
| numDims | Input | Number of dims dimensions to set. |
| dims | Input | Pointer to dims. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
