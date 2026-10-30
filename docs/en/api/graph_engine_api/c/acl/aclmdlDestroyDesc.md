# aclmdlDestroyDesc

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:04:57.274Z pushedAt=2026-10-29T03:01:28.868Z nl=2 -->

## Product Support

<!-- npu="950" id400 -->
- Ascend 950PR&950DT products: Supported
<!-- end id400 -->
<!-- npu="A3" id401 -->
- Atlas A3 products: Supported
<!-- end id401 -->
<!-- npu="910b" id402 -->
- Atlas A2 products: Supported
<!-- end id402 -->
<!-- npu="310b" id403 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id403 -->
<!-- npu="310p" id404 -->
- Atlas inference products: Supported
<!-- end id404 -->
<!-- npu="910" id405 -->
- Atlas training products: Supported
<!-- end id405 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlDestroyDesc_res.md#id1 -->

## Feature Description

Destroys the aclmdlDesc data created by the [aclmdlCreateDesc](aclmdlCreateDesc.md) API.

## Prototype

```c
aclError aclmdlDestroyDesc(aclmdlDesc *modelDesc)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| modelDesc | Input | Pointer to the aclmdlDesc object to be destroyed. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
