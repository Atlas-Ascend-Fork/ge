# aclmdlBundleLoadModel

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T14:55:15.997Z pushedAt=2026-10-29T02:56:01.634Z nl=2 -->

## Product Support

<!-- npu="950" id629 -->
- Ascend 950PR&950DT products: Supported
<!-- end id629 -->
<!-- npu="A3" id630 -->
- Atlas A3 products: Supported
<!-- end id630 -->
<!-- npu="910b" id631 -->
- Atlas A2 products: Supported
<!-- end id631 -->
<!-- npu="310b" id632 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id632 -->
<!-- npu="310p" id633 -->
- Atlas inference products: Supported
<!-- end id633 -->
<!-- npu="910" id634 -->
- Atlas training products: Supported
<!-- end id634 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlBundleLoadModel_res.md#id1 -->

## Feature Description

Loads a graph from the model based on the graph index. The memory is managed internally by the system.

## Prototype

```c
aclError aclmdlBundleLoadModel(uint32_t bundleId, size_t index, uint32_t *modelId)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| bundleId | Input | bundleId returned after the model is successfully initialized by [aclmdlBundleInitFromFile](aclmdlBundleInitFromFile.md) or [aclmdlBundleInitFromMem](aclmdlBundleInitFromMem.md). |
| index | Input | Index.<br>After you call [aclmdlBundleGetQueryModelNum](aclmdlBundleGetQueryModelNum.md) to obtain the total number of graphs, the value range of index is [0, (total number of graphs - 1)]. |
| modelId | Output | ID of the graph that can be actually executed. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
