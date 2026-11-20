# aclDestroyTensorDesc

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-11-17T13:35:34.674Z pushedAt=2026-11-18T03:07:08.898Z nl=2 -->

## Product Support

<!-- npu="950" id288 -->
- Ascend 950PR&950DT products: Supported
<!-- end id288 -->
<!-- npu="A3" id289 -->
- Atlas A3 products: Supported
<!-- end id289 -->
<!-- npu="910b" id290 -->
- Atlas A2 products: Supported
<!-- end id290 -->
<!-- npu="310b" id291 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id291 -->
<!-- npu="310p" id292 -->
- Atlas inference products: Supported
<!-- end id292 -->
<!-- npu="910" id293 -->
- Atlas training products: Supported
<!-- end id293 -->
<!-- npu="IPV350" id294 -->
- IPV350: not supported
<!-- end id294 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclDestroyTensorDesc_res.md#id1 -->

## Function Description

Destroys data of the aclTensorDesc type.

## Prototype

```c
void aclDestroyTensorDesc(const aclTensorDesc *desc)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| desc | Input | Pointer to the [aclTensorDesc](aclTensorDesc.md) to be destroyed. |

## Return Value

None
