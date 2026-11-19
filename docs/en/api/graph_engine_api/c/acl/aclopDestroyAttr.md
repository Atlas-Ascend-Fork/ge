# aclopDestroyAttr

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-11-17T15:00:52.926Z pushedAt=2026-11-18T03:41:47.702Z nl=2 -->

## Product Support

<!-- npu="950" id532 -->
- Ascend 950PR&950DT products: Supported
<!-- end id532 -->
<!-- npu="A3" id533 -->
- Atlas A3 products: Supported
<!-- end id533 -->
<!-- npu="910b" id534 -->
- Atlas A2 products: Supported
<!-- end id534 -->
<!-- npu="310b" id535 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id535 -->
<!-- npu="310p" id536 -->
- Atlas inference products: Supported
<!-- end id536 -->
<!-- npu="910" id537 -->
- Atlas training products: Supported
<!-- end id537 -->
<!-- npu="IPV350" id538 -->
- IPV350: not supported
<!-- end id538 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclopDestroyAttr_res.md#id1 -->

## Function Description

Destroys the aclopAttr data created through the [aclopCreateAttr](aclopCreateAttr.md) API.

## Prototype

```c
void aclopDestroyAttr(const aclopAttr *attr)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| attr | Input | aclopAttr pointer to be destroyed. |

## Return Value

None
