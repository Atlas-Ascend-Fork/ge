# aclmdlBundleInitFromFile

<!-- md-trans-meta sourceCommit=aaa82213df16687539020ed5cae4f006363bb63c translatedAt=2026-11-17T13:57:01.022Z pushedAt=2026-11-18T03:17:47.678Z nl=2 -->

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
<!-- npu="IPV350" id524 -->
- IPV350: not supported
<!-- end id524 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlBundleInitFromFile_res.md#id1 -->

## Function Description

During model execution of an om or om2 model, if dynamic variable updates are involved, call this API to initialize the model from the om/om2 model file.

## Prototype

```c
aclError aclmdlBundleInitFromFile(const char* modelPath, void *varWeightPtr, size_t varWeightSize, uint32_t *bundleId)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| modelPath | Input | Pointer to the model file path, which includes the file name. The user running the application (APP) must have access permission to the storage path.<br>The model file here is built using the graph construction API. Call aclgrphBundleBuildModel to compile the model and aclgrphBundleSaveModel to save the model. For details about these APIs, see [aclgrphBundleBuildModel](../../cpp/ge/aclgrphBundleBuildModel.md). |
| varWeightPtr | Input | Address pointer to the refreshable weight memory required by the model, managed by you. Do not release this memory during model execution.<br>If a null pointer is passed, the memory is managed by the system. |
| varWeightSize | Input | Size of the refreshable weight memory required by the model, in bytes. This parameter is invalid when varWeightPtr is a null pointer. |
| bundleId | Output | After the system successfully initializes the model, bundleId is returned as the identifier for recognizing the model in subsequent operations. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
