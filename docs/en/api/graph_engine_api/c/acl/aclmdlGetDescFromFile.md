# aclmdlGetDescFromFile

<!-- md-trans-meta sourceCommit=fcba9ddddeed35cd2a7a0ade347fab638d62fcb2 translatedAt=2026-11-17T14:19:10.522Z pushedAt=2026-11-18T03:30:34.938Z nl=2 -->

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
<!-- npu="IPV350" id1216 -->
- IPV350: support
<!-- end id1216 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlGetDescFromFile_res.md#id1 -->

## Function Description

Obtains the model description of a model from an OM/OM2 model file.

The model description obtained through this API cannot be used with the [aclmdlGetOpAttr](aclmdlGetOpAttr.md) and [aclmdlGetCurOutputDims](aclmdlGetCurOutputDims.md) APIs.

## Prototype

```c
aclError aclmdlGetDescFromFile(aclmdlDesc *modelDesc, const char *modelPath)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| modelDesc | Output | Pointer to the aclmdlDesc type.<br>Call the [aclmdlCreateDesc](aclmdlCreateDesc.md) API in advance to create data of the aclmdlDesc type. |
| modelPath | Input | Pointer to the model file path, which contains the file name. The user who runs the application (APP) must have access permission to the storage path.<br>The model file here is an OM or OM2 model file. For details about how to obtain a model file, see *[ATC Offline Model Compilation Tool](../../../../user_guides/atc_tools/README.md)*, specifically "[--mode](../../../../user_guides/atc_tools/CLI_options/--mode.md)". |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
