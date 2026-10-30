# aclGetTensorDescType

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T14:49:31.980Z pushedAt=2026-10-29T02:41:05.358Z nl=2 -->

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
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclGetTensorDescType_res.md#id1 -->

## Feature Description

Obtains the data type in the tensor description.

## Prototype

```c
aclDataType aclGetTensorDescType(const aclTensorDesc *desc)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| desc | Input | Pointer to the aclTensorDesc type.<br>Call the [aclCreateTensorDesc](aclCreateTensorDesc.md) API in advance to create the aclTensorDesc type. |

## Return Value

Returns the data type of the specified tensor description, which is of the aclDataType type.
