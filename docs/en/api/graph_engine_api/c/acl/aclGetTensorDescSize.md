# aclGetTensorDescSize

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T14:48:29.981Z pushedAt=2026-10-29T02:40:55.749Z nl=2 -->

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
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclGetTensorDescSize_res.md#id1 -->

## Feature Description

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
