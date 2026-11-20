# aclGetTensorDescType

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-11-17T13:51:41.000Z pushedAt=2026-11-18T03:15:33.211Z nl=2 -->

## Product Support

<!-- npu="950" id99 -->
- Ascend 950PR&950DT products: Supported
<!-- end id99 -->
<!-- npu="A3" id100 -->
- Atlas A3 products: Supported
<!-- end id100 -->
<!-- npu="910b" id101 -->
- Atlas A2 products: Supported
<!-- end id101 -->
<!-- npu="310b" id102 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id102 -->
<!-- npu="310p" id103 -->
- Atlas inference products: Supported
<!-- end id103 -->
<!-- npu="910" id104 -->
- Atlas training products: Supported
<!-- end id104 -->
<!-- npu="IPV350" id105 -->
- IPV350: not supported
<!-- end id105 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclGetTensorDescType_res.md#id1 -->

## Function Description

Obtains the data type in the tensor description.

## Prototype

```c
aclDataType aclGetTensorDescType(const aclTensorDesc *desc)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| desc | Input | Pointer to the aclTensorDesc type.<br>Call [aclCreateTensorDesc](aclCreateTensorDesc.md) in advance to create the aclTensorDesc type. |

## Return Value

Returns the data type of the specified tensor description, which is of the aclDataType type.
