# aclmdlCreateDesc

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-11-17T14:05:41.349Z pushedAt=2026-11-18T03:19:38.158Z nl=2 -->

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
<!-- npu="IPV350" id350 -->
- IPV350: support
<!-- end id350 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlCreateDesc_res.md#id1 -->

## Function Description

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
