# aclmdlBundleUnloadModel

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T14:58:12.976Z pushedAt=2026-10-29T02:57:00.858Z nl=2 -->

## Product Support

<!-- npu="950" id860 -->
- Ascend 950PR&950DT products: Supported
<!-- end id860 -->
<!-- npu="A3" id861 -->
- Atlas A3 products: Supported
<!-- end id861 -->
<!-- npu="910b" id862 -->
- Atlas A2 products: Supported
<!-- end id862 -->
<!-- npu="310b" id863 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id863 -->
<!-- npu="310p" id864 -->
- Atlas inference products: Supported
<!-- end id864 -->
<!-- npu="910" id865 -->
- Atlas training products: Supported
<!-- end id865 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlBundleUnloadModel_res.md#id1 -->

## Feature Description

Unloads a graph from the model.

## Prototype

```c
aclError aclmdlBundleUnloadModel(uint32_t bundleId, uint32_t modelId)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| bundleId | Input | bundleId returned after the model is successfully initialized by calling [aclmdlBundleInitFromFile](aclmdlBundleInitFromFile.md) or [aclmdlBundleInitFromMem](aclmdlBundleInitFromMem.md). |
| modelId | Input | ID of the graph to be unloaded. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
