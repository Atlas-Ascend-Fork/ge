# aclopCreateAttr

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:45:32.376Z pushedAt=2026-10-29T03:42:55.714Z nl=2 -->

## Product Support

<!-- npu="950" id1049 -->
- Ascend 950PR&950DT products: Supported
<!-- end id1049 -->
<!-- npu="A3" id1050 -->
- Atlas A3 products: Supported
<!-- end id1050 -->
<!-- npu="910b" id1051 -->
- Atlas A2 products: Supported
<!-- end id1051 -->
<!-- npu="310b" id1052 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id1052 -->
<!-- npu="310p" id1053 -->
- Atlas inference products: Supported
<!-- end id1053 -->
<!-- npu="910" id1054 -->
- Atlas training products: Supported
<!-- end id1054 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclopCreateAttr_res.md#id1 -->

## Feature Description

Creates data of the aclopAttr type, which is used to store operator attributes.

To destroy data of the aclopAttr type, see [aclopDestroyAttr](aclopDestroyAttr.md).

## Prototype

```c
aclopAttr *aclopCreateAttr()
```

## Parameters

None

## Return Value

Returns a pointer of the aclopAttr type.
