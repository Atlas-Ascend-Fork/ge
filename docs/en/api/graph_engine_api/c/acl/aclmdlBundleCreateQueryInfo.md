# aclmdlBundleCreateQueryInfo

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T14:51:04.238Z pushedAt=2026-10-29T02:53:58.594Z nl=2 -->

## Product Support

<!-- npu="950" id1175 -->
- Ascend 950PR&950DT products: Supported
<!-- end id1175 -->
<!-- npu="A3" id1176 -->
- Atlas A3 products: Supported
<!-- end id1176 -->
<!-- npu="910b" id1177 -->
- Atlas A2 products: Supported
<!-- end id1177 -->
<!-- npu="310b" id1178 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id1178 -->
<!-- npu="310p" id1179 -->
- Atlas inference products: Supported
<!-- end id1179 -->
<!-- npu="910" id1180 -->
- Atlas training products: Supported
<!-- end id1180 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlBundleCreateQueryInfo_res.md#id1 -->

## Feature Description

Creates data of the aclmdlBundleQueryInfo type, which represents model description information.

To destroy data of the aclmdlBundleQueryInfo type, see [aclmdlBundleDestroyQueryInfo](aclmdlBundleDestroyQueryInfo.md).

## Prototype

```c
aclmdlBundleQueryInfo *aclmdlBundleCreateQueryInfo()
```

## Parameters

None

## Return Value

Returns a pointer of the aclmdlBundleQueryInfo type.
