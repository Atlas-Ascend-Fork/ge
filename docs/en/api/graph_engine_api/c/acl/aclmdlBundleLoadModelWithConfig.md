# aclmdlBundleLoadModelWithConfig

<!-- md-trans-meta sourceCommit=aaa82213df16687539020ed5cae4f006363bb63c translatedAt=2026-11-17T13:59:15.032Z pushedAt=2026-11-18T03:18:18.725Z nl=2 -->

## Product Support

<!-- npu="950" id491 -->
- Ascend 950PR&950DT products: Supported
<!-- end id491 -->
<!-- npu="A3" id492 -->
- Atlas A3 products: Supported
<!-- end id492 -->
<!-- npu="910b" id493 -->
- Atlas A2 products: Supported
<!-- end id493 -->
<!-- npu="310b" id494 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id494 -->
<!-- npu="310p" id495 -->
- Atlas inference products: Supported
<!-- end id495 -->
<!-- npu="910" id496 -->
- Atlas training products: Supported
<!-- end id496 -->
<!-- npu="IPV350" id497 -->
- IPV350: not supported
<!-- end id497 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlBundleLoadModelWithConfig_res.md#id1 -->

## Function Description

Loads the graph in the om/om2 model by graph index, and allows you to specify loading configuration information.

## Prototype

```c
aclError aclmdlBundleLoadModelWithConfig(uint32_t bundleId, size_t index, aclmdlConfigHandle *handle, uint32_t *modelId)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| bundleId | Input | bundleId returned after the model is successfully initialized by calling [aclmdlBundleInitFromFile](aclmdlBundleInitFromFile.md) or [aclmdlBundleInitFromMem](aclmdlBundleInitFromMem.md). |
| index | Input | Index.<br>After you call the [aclmdlBundleGetQueryModelNum](aclmdlBundleGetQueryModelNum.md) API to obtain the total number of graphs, the value range of index is [0, (total number of graphs - 1)]. |
| handle | Input | Pointer to the configuration object for model loading. Call [aclmdlCreateConfigHandle](aclmdlCreateConfigHandle.md) in advance to create this object, and keep it consistent with the handle in aclmdlSetConfigOpt. |
| modelId | Output | ID of the graph that can actually be executed. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
