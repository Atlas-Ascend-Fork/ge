# aclmdlBundleLoadFromMem

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T14:55:13.210Z pushedAt=2026-10-29T02:55:53.266Z nl=2 -->

## Product Support

<!-- npu="950" id1154 -->
- Ascend 950PR&950DT products: Supported
<!-- end id1154 -->
<!-- npu="A3" id1155 -->
- Atlas A3 products: Supported
<!-- end id1155 -->
<!-- npu="910b" id1156 -->
- Atlas A2 products: Supported
<!-- end id1156 -->
<!-- npu="310b" id1157 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id1157 -->
<!-- npu="310p" id1158 -->
- Atlas inference products: Supported
<!-- end id1158 -->
<!-- npu="910" id1159 -->
- Atlas training products: Supported
<!-- end id1159 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlBundleLoadFromMem_res.md#id1 -->

## Feature Description

Loads offline model data from memory for scenarios where variables are dynamically updated during model execution. The system manages the memory used for model running.

**This API must be used together with other APIs** to dynamically update variables. For details, see [aclmdlBundleLoadFromFile](aclmdlBundleLoadFromFile.md).

## Prototype

```c
aclError aclmdlBundleLoadFromMem(const void *model,  size_t modelSize, uint32_t *bundleId)
```

## Parameters

| parameter name | Input/Output | Description |
| --- | --- | --- |
| model | Input | Memory address pointer that stores the model data.<br>The model file is built using the graph construction API. Call the aclgrphBundleBuildModel API to compile the model, call the aclgrphBundleSaveModel API to save the model, and then read the saved .om model file into memory yourself. |
| modelSize | Input | Length of the model data in memory, in bytes. |
| bundleId | Output | After the model is loaded successfully, the system returns bundleId as the identifier for recognizing the model in subsequent operations. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).

## Constraints

<!-- npu="950,A3,910b,910,310p,310b" id1 -->
In Ascend EP mode, the memory for the model parameter must be allocated on the Host.
<!-- end id1 -->

<!-- npu="310b" id2 -->
In Ascend RC mode, the memory for the model parameter must be allocated on the Device.
<!-- end id2 -->

<!-- npu="310p" id3 -->
In Control CPU open mode, the memory for the model parameter must be allocated on the Device.
<!-- end id3 -->

<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlBundleLoadFromMem_res.md#id2 -->
