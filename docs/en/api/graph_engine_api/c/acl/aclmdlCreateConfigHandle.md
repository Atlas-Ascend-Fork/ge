# aclmdlCreateConfigHandle

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:00:04.212Z pushedAt=2026-10-29T02:59:56.305Z nl=2 -->

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
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlCreateConfigHandle_res.md#id1 -->

## Feature Description

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
