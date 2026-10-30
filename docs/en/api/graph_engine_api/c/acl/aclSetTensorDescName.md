# aclSetTensorDescName

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T16:00:58.717Z pushedAt=2026-10-29T03:59:23.113Z nl=2 -->

## Product Support

<!-- npu="950" id1112 -->
- Ascend 950PR&950DT products: Supported
<!-- end id1112 -->
<!-- npu="A3" id1113 -->
- Atlas A3 products: Supported
<!-- end id1113 -->
<!-- npu="910b" id1114 -->
- Atlas A2 products: Supported
<!-- end id1114 -->
<!-- npu="310b" id1115 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id1115 -->
<!-- npu="310p" id1116 -->
- Atlas inference products: Supported
<!-- end id1116 -->
<!-- npu="910" id1117 -->
- Atlas training products: Supported
<!-- end id1117 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclSetTensorDescName_res.md#id1 -->

## Feature Description

Sets the name of a TensorDesc.

## Prototype

```c
void aclSetTensorDescName(aclTensorDesc *desc, const char *name)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| desc | Output | Pointer to the aclTensorDesc type.<br>Call [aclCreateTensorDesc](aclCreateTensorDesc.md) in advance to create the aclTensorDesc type. |
| name | Input | Pointer to the TensorDesc name. |

## Return Value

None
