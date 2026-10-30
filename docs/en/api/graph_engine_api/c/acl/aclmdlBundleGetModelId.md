# aclmdlBundleGetModelId

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T14:51:39.289Z pushedAt=2026-10-29T02:54:18.890Z nl=2 -->

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
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlBundleGetModelId_res.md#id1 -->

## Feature Description

Obtains the ID of the model that can be actually executed.

**This API must be used together with other APIs** to dynamically update variables. For details, see the description in [aclmdlBundleLoadFromFile](aclmdlBundleLoadFromFile.md).

## Prototype

```c
aclError aclmdlBundleGetModelId(uint32_t bundleId, size_t index, uint32_t *modelId)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| bundleId | Input | bundleId returned after the model is loaded successfully by calling the [aclmdlBundleLoadFromFile](aclmdlBundleLoadFromFile.md) API or the [aclmdlBundleLoadFromMem](aclmdlBundleLoadFromMem.md) API. |
| index | Input | Index.<br>After you call the [aclmdlBundleGetModelNum](aclmdlBundleGetModelNum.md) API to obtain the number of models, the value range of index is [0, (number of models - 1)]. |
| modelId | Output | ID of the model that can be actually executed. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
