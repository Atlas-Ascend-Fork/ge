# aclSetTensorDescName

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-11-17T15:18:08.625Z pushedAt=2026-11-18T06:15:10.580Z nl=2 -->

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
<!-- npu="IPV350" id1118 -->
- IPV350: not supported
<!-- end id1118 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclSetTensorDescName_res.md#id1 -->

## Function Description

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
