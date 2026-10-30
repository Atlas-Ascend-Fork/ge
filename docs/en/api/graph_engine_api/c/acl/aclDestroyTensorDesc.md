# aclDestroyTensorDesc

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T14:36:46.445Z pushedAt=2026-10-29T02:33:08.745Z nl=2 -->

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
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclDestroyTensorDesc_res.md#id1 -->

## Feature Description

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
