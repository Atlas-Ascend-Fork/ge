# aclGetTensorDescDim (Deprecated)

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T14:45:53.484Z pushedAt=2026-10-29T02:39:33.315Z nl=2 -->

**Note: This API will be deprecated in later versions. Use [aclGetTensorDescDimV2](aclGetTensorDescDimV2.md) instead.**

## Product Support

<!-- npu="950" id449 -->
- Ascend 950PR&950DT products: Supported
<!-- end id449 -->
<!-- npu="A3" id450 -->
- Atlas A3 products: Supported
<!-- end id450 -->
<!-- npu="910b" id451 -->
- Atlas A2 products: Supported
<!-- end id451 -->
<!-- npu="310b" id452 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id452 -->
<!-- npu="310p" id453 -->
- Atlas inference products: Supported
<!-- end id453 -->
<!-- npu="910" id454 -->
- Atlas training products: Supported
<!-- end id454 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclGetTensorDescDim_deprecated_res.md#id1 -->

## Feature Description

Obtains the size of a specified dimension in the tensor description.

## Prototype

```c
int64_t aclGetTensorDescDim(const aclTensorDesc *desc, size_t index)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| desc | Input | Pointer to the aclTensorDesc type.<br>Call the [aclCreateTensorDesc](aclCreateTensorDesc.md) API in advance to create the aclTensorDesc type. |
| index | Input | Specifies the dimension whose size is to be obtained. The index value starts from 0.<br>Call the [aclGetTensorDescNumDims](aclGetTensorDescNumDims.md) API to obtain the number of shape dimensions. The value range of this index is [0, (number of shape dimensions - 1)]. |

## Return Value

Returns the size of the specified dimension in the specified tensor description.
