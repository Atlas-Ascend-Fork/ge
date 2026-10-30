# aclmdlBundleQueryInfoFromFile

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T14:56:41.198Z pushedAt=2026-10-29T02:56:28.836Z nl=2 -->

## Product Support

<!-- npu="950" id657 -->
- Ascend 950PR&950DT products: Supported
<!-- end id657 -->
<!-- npu="A3" id658 -->
- Atlas A3 products: Supported
<!-- end id658 -->
<!-- npu="910b" id659 -->
- Atlas A2 products: Supported
<!-- end id659 -->
<!-- npu="310b" id660 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id660 -->
<!-- npu="310p" id661 -->
- Atlas inference products: Supported
<!-- end id661 -->
<!-- npu="910" id662 -->
- Atlas training products: Supported
<!-- end id662 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlBundleQueryInfoFromFile_res.md#id1 -->

## Feature Description

During model execution, if dynamically updated variables are involved, you can call this API to obtain their description information from the model file.

## Prototype

```c
aclError aclmdlBundleQueryInfoFromFile(const char* fileName, aclmdlBundleQueryInfo *queryInfo)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| fileName | Input | Pointer to the model file path, which includes the file name. The user running the application (APP) must have access permission to the storage path.<br>The model file here is built using the graph construction API. Call aclgrphBundleBuildModel to compile the model and aclgrphBundleSaveModel to save the model. For details about these APIs, see [aclgrphBundleBuildModel](../../cpp/ge/aclgrphBundleBuildModel.md). |
| queryInfo | Output | Call [aclmdlBundleCreateQueryInfo](aclmdlBundleCreateQueryInfo.md) in advance to create data of the aclmdlBundleQueryInfo type. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
