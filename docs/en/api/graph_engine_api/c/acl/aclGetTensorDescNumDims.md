# aclGetTensorDescNumDims

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-11-17T13:50:41.779Z pushedAt=2026-11-18T03:11:17.555Z nl=2 -->

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
<!-- npu="IPV350" id600 -->
- IPV350: not supported
<!-- end id600 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclGetTensorDescNumDims_res.md#id1 -->

## Function Description

Obtains the number of dimensions of the shape in a tensor descriptor.

## Prototype

```c
size_t aclGetTensorDescNumDims(const aclTensorDesc *desc)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| desc | Input | Pointer to the aclTensorDesc type.<br>Call [aclCreateTensorDesc](aclCreateTensorDesc.md) in advance to create the aclTensorDesc type. |

## Return Value

Returns the number of dimensions of the shape in the tensor descriptor. If ACL\_UNKNOWN\_RANK (\#define ACL\_UNKNOWN\_RANK 0xFFFFFFFFFFFFFFFE) is returned, the number of dimensions is unknown in a dynamic shape scenario.
