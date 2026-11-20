# aclmdlBundleDestroyQueryInfo

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-11-17T13:53:59.822Z pushedAt=2026-11-18T03:17:11.962Z nl=2 -->

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
<!-- npu="IPV350" id357 -->
- IPV350: not supported
<!-- end id357 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlBundleDestroyQueryInfo_res.md#id1 -->

## Function Description

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
