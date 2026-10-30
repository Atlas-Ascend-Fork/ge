# aclGetTensorDescName

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T14:47:43.683Z pushedAt=2026-10-29T02:40:25.870Z nl=2 -->

## Product Support

<!-- npu="950" id1 -->
- Ascend 950PR&950DT products: Supported
<!-- end id1 -->
<!-- npu="A3" id2 -->
- Atlas A3 products: Supported
<!-- end id2 -->
<!-- npu="910b" id3 -->
- Atlas A2 products: Supported
<!-- end id3 -->
<!-- npu="310b" id4 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id4 -->
<!-- npu="310p" id5 -->
- Atlas inference products: Supported
<!-- end id5 -->
<!-- npu="910" id6 -->
- Atlas training products: Supported
<!-- end id6 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclGetTensorDescName_res.md#id1 -->

## Feature Description

Obtains the name of the TensorDesc.

## Prototype

```c
const char *aclGetTensorDescName(aclTensorDesc *desc)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| desc | Input | Pointer to the aclTensorDesc type.<br>Call the [aclCreateTensorDesc](aclCreateTensorDesc.md) API in advance to create the aclTensorDesc type. |

## Return Value

Returns the name of the TensorDesc.
