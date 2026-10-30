# aclmdlBundleInitFromFile

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T14:53:46.568Z pushedAt=2026-10-29T02:55:05.390Z nl=2 -->

## Product Support

<!-- npu="950" id518 -->
- Ascend 950PR&950DT products: Supported
<!-- end id518 -->
<!-- npu="A3" id519 -->
- Atlas A3 products: Supported
<!-- end id519 -->
<!-- npu="910b" id520 -->
- Atlas A2 products: Supported
<!-- end id520 -->
<!-- npu="310b" id521 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id521 -->
<!-- npu="310p" id522 -->
- Atlas inference products: Supported
<!-- end id522 -->
<!-- npu="910" id523 -->
- Atlas training products: Supported
<!-- end id523 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlBundleInitFromFile_res.md#id1 -->

## Feature Description

During model execution, you can call this API to initialize the model from a model file in scenarios involving dynamic variable updates.

## Prototype

```c
aclError aclmdlBundleInitFromFile(const char* modelPath, void *varWeightPtr, size_t varWeightSize, uint32_t *bundleId)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| modelPath | Input | Pointer to the model file path, which includes the file name. The user running the application (APP) must have access permission to this storage path.<br>The model file here is built through the graph building API. Call the aclgrphBundleBuildModel API to compile the model and the aclgrphBundleSaveModel API to save the model. For details about these APIs, see [aclgrphBundleBuildModel](../../cpp/ge/aclgrphBundleBuildModel.md). |
| varWeightPtr | Input | Address pointer to the refreshable weight memory required by the model. It is managed by you and must not be released during model execution.<br>If a null pointer is passed here, the memory is managed by the system. |
| varWeightSize | Input | Size of the refreshable weight memory required by the model, in bytes. This parameter is invalid when varWeightPtr is a null pointer. |
| bundleId | Output | After the system successfully initializes the model, bundleId is returned as the identifier for recognizing the model in subsequent operations. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
