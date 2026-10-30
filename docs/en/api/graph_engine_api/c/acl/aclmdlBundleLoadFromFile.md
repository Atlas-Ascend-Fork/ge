# aclmdlBundleLoadFromFile

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T14:54:35.631Z pushedAt=2026-10-29T02:55:45.578Z nl=2 -->

## Product Support

<!-- npu="950" id358 -->
- Ascend 950PR&950DT products: Supported
<!-- end id358 -->
<!-- npu="A3" id359 -->
- Atlas A3 products: Supported
<!-- end id359 -->
<!-- npu="910b" id360 -->
- Atlas A2 products: Supported
<!-- end id360 -->
<!-- npu="310b" id361 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id361 -->
<!-- npu="310p" id362 -->
- Atlas inference products: Supported
<!-- end id362 -->
<!-- npu="910" id363 -->
- Atlas training products: Supported
<!-- end id363 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlBundleLoadFromFile_res.md#id1 -->

## Feature Description

If the model execution phase involves dynamically updating variables, call this API to load the om model from a file, with memory managed internally by the system.

**This API must be used together with the following APIs** to dynamically update variables. The calling process of the key APIs is as follows:

1. Compile and save the model based on the graph construction APIs. The model contains multiple graphs, such as the inference graph, variable initialization graph, and variable update graph.

    Here, call the aclgrphBundleBuildModel API to compile the model and the aclgrphBundleSaveModel API to save the model. For details about these APIs, see [aclgrphBundleBuildModel](../../cpp/ge/aclgrphBundleBuildModel.md) and [aclgrphBundleSaveModel](../../cpp/ge/aclgrphBundleSaveModel.md).

2. Call the [aclmdlBundleLoadFromFile](aclmdlBundleLoadFromFile.md) or [aclmdlBundleLoadFromMem](aclmdlBundleLoadFromMem.md) API to load the model.
3. Call the [aclmdlBundleGetModelId](aclmdlBundleGetModelId.md) API to obtain the IDs of multiple graphs.
4. Based on the IDs of the multiple graphs, call the model execution API (for example, [aclmdlExecute](aclmdlExecute.md)) to execute each graph.

    If variable update is involved, before executing the variable update graph, call the [aclmdlSetDatasetTensorDesc](aclmdlSetDatasetTensorDesc.md) API to set the tensor description of the graph, then execute the variable update graph, and then execute the inference graph again.

5. After inference is complete, call the [aclmdlBundleUnload](aclmdlBundleUnload.md) API to unload the model.

## Prototype

```c
aclError aclmdlBundleLoadFromFile(const char *modelPath, uint32_t *bundleId)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| modelPath | Input | Pointer to the model file path, including the file name. The user running the application (APP) must have access permission to the storage path.<br>The model file here is built through the graph construction APIs. Call aclgrphBundleBuildModel to compile the model and aclgrphBundleSaveModel to save the model. |
| bundleId | Output | After the system successfully loads the model, bundleId is returned as the identifier for recognizing the model in subsequent operations. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
