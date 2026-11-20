# aclmdlBundleGetModelId

<!-- md-trans-meta sourceCommit=aaa82213df16687539020ed5cae4f006363bb63c translatedAt=2026-11-17T13:54:23.315Z pushedAt=2026-11-18T03:17:19.054Z nl=2 -->

## Product Support

<!-- npu="950" id769 -->
- Ascend 950PR&950DT products: Supported
<!-- end id769 -->
<!-- npu="A3" id770 -->
- Atlas A3 products: Supported
<!-- end id770 -->
<!-- npu="910b" id771 -->
- Atlas A2 products: Supported
<!-- end id771 -->
<!-- npu="310b" id772 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id772 -->
<!-- npu="310p" id773 -->
- Atlas inference products: Supported
<!-- end id773 -->
<!-- npu="910" id774 -->
- Atlas training products: Supported
<!-- end id774 -->
<!-- npu="IPV350" id775 -->
- IPV350: not supported
<!-- end id775 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlBundleGetModelId_res.md#id1 -->

## Function Description

Obtains the ID of the actually executable om/om2 model.

**This API must be used together with other APIs** to dynamically update variables. For details, see the description of [aclmdlBundleLoadFromFile](aclmdlBundleLoadFromFile.md).

## Prototype

```c
aclError aclmdlBundleGetModelId(uint32_t bundleId, size_t index, uint32_t *modelId)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| bundleId | Input | bundleId returned after the model is successfully loaded by the [aclmdlBundleLoadFromFile](aclmdlBundleLoadFromFile.md) API or the [aclmdlBundleLoadFromMem](aclmdlBundleLoadFromMem.md) API. |
| index | Input | Index.<br>After you call the [aclmdlBundleGetModelNum](aclmdlBundleGetModelNum.md) API to obtain the number of models, the value range of index is [0, (number of models - 1)]. |
| modelId | Output | ID of the model that can be actually executed. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
