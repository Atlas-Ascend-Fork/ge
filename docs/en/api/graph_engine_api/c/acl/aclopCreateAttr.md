# aclopCreateAttr

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-11-17T14:59:28.350Z pushedAt=2026-11-18T03:41:01.390Z nl=2 -->

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
<!-- npu="IPV350" id1055 -->
- IPV350: not supported
<!-- end id1055 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclopCreateAttr_res.md#id1 -->

## Function Description

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
