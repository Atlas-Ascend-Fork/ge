# aclopDestroyHandle

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-11-17T15:01:12.691Z pushedAt=2026-11-18T03:41:52.218Z nl=2 -->

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
<!-- npu="IPV350" id1090 -->
- IPV350: not supported
<!-- end id1090 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclopDestroyHandle_res.md#id1 -->

## Function Description

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
