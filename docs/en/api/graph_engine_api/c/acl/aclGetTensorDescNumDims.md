# aclGetTensorDescNumDims

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T14:48:24.157Z pushedAt=2026-10-29T02:40:44.168Z nl=2 -->

## Product Support

<!-- npu="950" id594 -->
- Ascend 950PR&950DT products: Supported
<!-- end id594 -->
<!-- npu="A3" id595 -->
- Atlas A3 products: Supported
<!-- end id595 -->
<!-- npu="910b" id596 -->
- Atlas A2 products: Supported
<!-- end id596 -->
<!-- npu="310b" id597 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id597 -->
<!-- npu="310p" id598 -->
- Atlas inference products: Supported
<!-- end id598 -->
<!-- npu="910" id599 -->
- Atlas training products: Supported
<!-- end id599 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclGetTensorDescNumDims_res.md#id1 -->

## Feature Description

Obtains the number of dimensions of the shape in a tensor descriptor.

## Prototype

```c
size_t aclGetTensorDescNumDims(const aclTensorDesc *desc)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| desc | Input | Pointer to the aclTensorDesc type.<br>Call [aclCreateTensorDesc](aclCreateTensorDesc.md) to create the aclTensorDesc type in advance. |

## Return Value

Returns the number of dimensions of the shape in the tensor descriptor. If ACL\_UNKNOWN\_RANK (\#define ACL\_UNKNOWN\_RANK 0xFFFFFFFFFFFFFFFE) is returned, the number of dimensions is unknown in a dynamic shape scenario.
