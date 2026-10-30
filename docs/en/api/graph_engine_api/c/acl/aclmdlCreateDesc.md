# aclmdlCreateDesc

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:01:41.240Z pushedAt=2026-10-29T03:00:13.672Z nl=2 -->

## Product Support

<!-- npu="950" id344 -->
- Ascend 950PR&950DT products: Supported
<!-- end id344 -->
<!-- npu="A3" id345 -->
- Atlas A3 products: Supported
<!-- end id345 -->
<!-- npu="910b" id346 -->
- Atlas A2 products: Supported
<!-- end id346 -->
<!-- npu="310b" id347 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id347 -->
<!-- npu="310p" id348 -->
- Atlas inference products: Supported
<!-- end id348 -->
<!-- npu="910" id349 -->
- Atlas training products: Supported
<!-- end id349 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlCreateDesc_res.md#id1 -->

## Feature Description

Creates data of the aclmdlDesc type to describe model information.

To destroy data of the aclmdlDesc type, see [aclmdlDestroyDesc](aclmdlDestroyDesc.md).

## Prototype

```c
aclmdlDesc* aclmdlCreateDesc()
```

## Parameters

None

## Return Value

Returns a pointer of the aclmdlDesc type.
