# aclmdlCreateConfigHandle

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-11-17T14:04:23.958Z pushedAt=2026-11-18T03:19:20.897Z nl=2 -->

## Product Support

<!-- npu="950" id274 -->
- Ascend 950PR&950DT products: Supported
<!-- end id274 -->
<!-- npu="A3" id275 -->
- Atlas A3 products: Supported
<!-- end id275 -->
<!-- npu="910b" id276 -->
- Atlas A2 products: Supported
<!-- end id276 -->
<!-- npu="310b" id277 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id277 -->
<!-- npu="310p" id278 -->
- Atlas inference products: Supported
<!-- end id278 -->
<!-- npu="910" id279 -->
- Atlas training products: Supported
<!-- end id279 -->
<!-- npu="IPV350" id280 -->
- IPV350: support
<!-- end id280 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlCreateConfigHandle_res.md#id1 -->

## Function Description

Creates data of the aclmdlConfigHandle type, which represents a configuration object for model loading.

To destroy data of the aclmdlConfigHandle type, see [aclmdlDestroyConfigHandle](aclmdlDestroyConfigHandle.md).

## Prototype

```c
aclmdlConfigHandle *aclmdlCreateConfigHandle()
```

## Parameters

None

## Return Value

- Returns a pointer of the aclmdlConfigHandle type, indicating success.
- Returns nullptr, indicating failure.
