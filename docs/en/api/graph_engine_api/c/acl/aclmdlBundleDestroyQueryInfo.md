# aclmdlBundleDestroyQueryInfo

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T14:51:31.232Z pushedAt=2026-10-29T02:54:08.872Z nl=2 -->

## Product Support

<!-- npu="950" id351 -->
- Ascend 950PR&950DT products: Supported
<!-- end id351 -->
<!-- npu="A3" id352 -->
- Atlas A3 products: Supported
<!-- end id352 -->
<!-- npu="910b" id353 -->
- Atlas A2 products: Supported
<!-- end id353 -->
<!-- npu="310b" id354 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id354 -->
<!-- npu="310p" id355 -->
- Atlas inference products: Supported
<!-- end id355 -->
<!-- npu="910" id356 -->
- Atlas training products: Supported
<!-- end id356 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlBundleDestroyQueryInfo_res.md#id1 -->

## Feature Description

Destroys the aclmdlBundleQueryInfo data created by the [aclmdlBundleCreateQueryInfo](aclmdlBundleCreateQueryInfo.md) API.

## Prototype

```c
aclError aclmdlBundleDestroyQueryInfo(aclmdlBundleQueryInfo *queryInfo)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| queryInfo | Input | Pointer to the aclmdlBundleQueryInfo object to be destroyed. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
