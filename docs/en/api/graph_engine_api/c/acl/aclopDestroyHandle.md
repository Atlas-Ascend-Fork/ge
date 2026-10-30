# aclopDestroyHandle

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:47:22.011Z pushedAt=2026-10-29T03:43:34.513Z nl=2 -->

## Product Support

<!-- npu="950" id1084 -->
- Ascend 950PR&950DT products: Supported
<!-- end id1084 -->
<!-- npu="A3" id1085 -->
- Atlas A3 products: Supported
<!-- end id1085 -->
<!-- npu="910b" id1086 -->
- Atlas A2 products: Supported
<!-- end id1086 -->
<!-- npu="310b" id1087 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id1087 -->
<!-- npu="310p" id1088 -->
- Atlas inference products: Supported
<!-- end id1088 -->
<!-- npu="910" id1089 -->
- Atlas training products: Supported
<!-- end id1089 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclopDestroyHandle_res.md#id1 -->

## Feature Description

Destroys the handle for executing an operator that is created by the [aclopCreateHandle](aclopCreateHandle.md) API.

## Prototype

```c
void aclopDestroyHandle(aclopHandle *handle)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| handle | Input | Pointer to the aclopHandle to be destroyed. |

## Return Value

None
