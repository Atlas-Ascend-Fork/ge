# aclmdlBundleLoadModel

<!-- md-trans-meta sourceCommit=aaa82213df16687539020ed5cae4f006363bb63c translatedAt=2026-11-17T13:59:02.697Z pushedAt=2026-11-18T03:18:12.813Z nl=2 -->

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
<!-- npu="IPV350" id635 -->
- IPV350: not supported
<!-- end id635 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlBundleLoadModel_res.md#id1 -->

## Function Description

Loads a graph from an om/om2 model based on the graph index. The memory is managed internally by the system.

## Prototype

```c
aclError aclmdlBundleLoadModel(uint32_t bundleId, size_t index, uint32_t *modelId)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| bundleId | Input | bundleId returned after the model is successfully initialized by calling [aclmdlBundleInitFromFile](aclmdlBundleInitFromFile.md) or [aclmdlBundleInitFromMem](aclmdlBundleInitFromMem.md). |
| index | Input | Index.<br>After you call the [aclmdlBundleGetQueryModelNum](aclmdlBundleGetQueryModelNum.md) API to obtain the total number of graphs, the value range of index is [0, (total number of graphs - 1)]. |
| modelId | Output | ID of the graph that can actually be executed. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
