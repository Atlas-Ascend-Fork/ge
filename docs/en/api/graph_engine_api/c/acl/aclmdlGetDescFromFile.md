# aclmdlGetDescFromFile

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:12:42.682Z pushedAt=2026-10-29T03:12:05.003Z nl=2 -->

## Product Support

<!-- npu="950" id1210 -->
- Ascend 950PR&950DT products: Supported
<!-- end id1210 -->
<!-- npu="A3" id1211 -->
- Atlas A3 products: Supported
<!-- end id1211 -->
<!-- npu="910b" id1212 -->
- Atlas A2 products: Supported
<!-- end id1212 -->
<!-- npu="310b" id1213 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id1213 -->
<!-- npu="310p" id1214 -->
- Atlas inference products: Supported
<!-- end id1214 -->
<!-- npu="910" id1215 -->
- Atlas training products: Supported
<!-- end id1215 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlGetDescFromFile_res.md#id1 -->

## Feature Description

Obtains the model description information of a model based on the model file.

The model description information obtained through this API cannot be used with the [aclmdlGetOpAttr](aclmdlGetOpAttr.md) and [aclmdlGetCurOutputDims](aclmdlGetCurOutputDims.md) APIs.

## Prototype

```c
aclError aclmdlGetDescFromFile(aclmdlDesc *modelDesc, const char *modelPath)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| modelDesc | Output | Pointer to the aclmdlDesc type.<br>Call [aclmdlCreateDesc](aclmdlCreateDesc.md) in advance to create data of the aclmdlDesc type. |
| modelPath | Input | Pointer to the model file path, which contains the file name. The user who runs the application (APP) must have access permission to the storage path.<br>The model file here is an OM model file. For details about how to obtain an OM model file, see *[ATC Offline Model Compilation Tool](../../../../user_guides/atc_tools/README.md)*, specifically "[--mode](../../../../user_guides/atc_tools/CLI_options/--mode.md)". |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
