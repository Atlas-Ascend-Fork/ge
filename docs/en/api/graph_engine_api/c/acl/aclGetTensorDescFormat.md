# aclGetTensorDescFormat

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-11-17T13:49:39.952Z pushedAt=2026-11-18T03:11:06.462Z nl=2 -->

## Product Support

<!-- npu="950" id43 -->
- Ascend 950PR&950DT products: Supported
<!-- end id43 -->
<!-- npu="A3" id44 -->
- Atlas A3 products: Supported
<!-- end id44 -->
<!-- npu="910b" id45 -->
- Atlas A2 products: Supported
<!-- end id45 -->
<!-- npu="310b" id46 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id46 -->
<!-- npu="310p" id47 -->
- Atlas inference products: Supported
<!-- end id47 -->
<!-- npu="910" id48 -->
- Atlas training products: Supported
<!-- end id48 -->
<!-- npu="IPV350" id49 -->
- IPV350: not supported
<!-- end id49 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclGetTensorDescFormat_res.md#id1 -->

## Function Description

Obtains the format in the tensor description.

## Prototype

```c
aclFormat aclGetTensorDescFormat(const aclTensorDesc *desc)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| desc | Input | Pointer to the aclTensorDesc type.<br>Call [aclCreateTensorDesc](aclCreateTensorDesc.md) in advance to create the aclTensorDesc type. |

## Return Value

Returns the format of the specified tensor description, of the aclFormat type.
