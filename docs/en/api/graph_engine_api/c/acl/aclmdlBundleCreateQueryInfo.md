# aclmdlBundleCreateQueryInfo

<!-- md-trans-meta sourceCommit=aaa82213df16687539020ed5cae4f006363bb63c translatedAt=2026-11-17T13:53:33.629Z pushedAt=2026-11-18T03:16:03.129Z nl=2 -->

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
<!-- npu="IPV350" id1181 -->
- IPV350: not supported
<!-- end id1181 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlBundleCreateQueryInfo_res.md#id1 -->

## Function Description

Creates data of the aclmdlBundleQueryInfo type, which represents om/om2 model description information.

To destroy data of the aclmdlBundleQueryInfo type, see [aclmdlBundleDestroyQueryInfo](aclmdlBundleDestroyQueryInfo.md).

## Prototype

```c
aclmdlBundleQueryInfo *aclmdlBundleCreateQueryInfo()
```

## Parameters

None

## Return Value

Returns a pointer of the aclmdlBundleQueryInfo type.
