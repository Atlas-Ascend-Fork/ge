# aclmdlBundleGetModelNum

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T14:51:55.176Z pushedAt=2026-10-29T02:54:28.292Z nl=2 -->

## Product Support

<!-- npu="950" id811 -->
- Ascend 950PR&950DT products: Supported
<!-- end id811 -->
<!-- npu="A3" id812 -->
- Atlas A3 products: Supported
<!-- end id812 -->
<!-- npu="910b" id813 -->
- Atlas A2 products: Supported
<!-- end id813 -->
<!-- npu="310b" id814 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id814 -->
<!-- npu="310p" id815 -->
- Atlas inference products: Supported
<!-- end id815 -->
<!-- npu="910" id816 -->
- Atlas training products: Supported
<!-- end id816 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlBundleGetModelNum_res.md#id1 -->

## Feature Description

Obtains the actual number of executable graphs based on bundleId.

## Prototype

```c
aclError aclmdlBundleGetModelNum(uint32_t bundleId, size_t *modelNum)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| bundleId | Input | bundleId returned after the model is successfully loaded by the [aclmdlBundleLoadFromFile](aclmdlBundleLoadFromFile.md) API or the [aclmdlBundleLoadFromMem](aclmdlBundleLoadFromMem.md) API. |
| modelNum | Output | Actual number of executable graphs. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
