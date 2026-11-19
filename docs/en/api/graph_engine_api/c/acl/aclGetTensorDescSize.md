# aclGetTensorDescSize

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-11-17T13:51:15.708Z pushedAt=2026-11-18T03:11:31.380Z nl=2 -->

## Product Support

<!-- npu="950" id127 -->
- Ascend 950PR&950DT products: Supported
<!-- end id127 -->
<!-- npu="A3" id128 -->
- Atlas A3 products: Supported
<!-- end id128 -->
<!-- npu="910b" id129 -->
- Atlas A2 products: Supported
<!-- end id129 -->
<!-- npu="310b" id130 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id130 -->
<!-- npu="310p" id131 -->
- Atlas inference products: Supported
<!-- end id131 -->
<!-- npu="910" id132 -->
- Atlas training products: Supported
<!-- end id132 -->
<!-- npu="IPV350" id133 -->
- IPV350: not supported
<!-- end id133 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclGetTensorDescSize_res.md#id1 -->

## Function Description

Obtains the size of the space occupied by tensor data.

## Prototype

```c
size_t aclGetTensorDescSize(const aclTensorDesc *desc)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| desc | Input | Pointer to the aclTensorDesc type.<br>Call [aclCreateTensorDesc](aclCreateTensorDesc.md) in advance to create the aclTensorDesc type. |

## Return Value

Returns the size of the space occupied by the specified tensor description.
