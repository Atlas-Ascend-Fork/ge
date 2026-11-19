# aclmdlQueryWorkspaceSize

<!-- md-trans-meta sourceCommit=952f341a9689e98183fe9174c116ff95ab991801 translatedAt=2026-11-17T14:41:39.213Z pushedAt=2026-11-18T03:36:05.722Z nl=2 -->

## Product Support

<!-- npu="950" id1 -->
- Ascend 950PR&950DT products: Supported
<!-- end id1 -->
<!-- npu="A3" id2 -->
- Atlas A3 products: Supported
<!-- end id2 -->
<!-- npu="910b" id3 -->
- Atlas A2 products: Supported
<!-- end id3 -->
<!-- npu="310b" id4 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id4 -->
<!-- npu="310p" id5 -->
- Atlas inference products: Supported
<!-- end id5 -->
<!-- npu="910" id6 -->
- Atlas training products: Supported
<!-- end id6 -->
<!-- npu="IPV350" id7 -->
- IPV350: not supported
<!-- end id7 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlQueryWorkspaceSize_res.md#id1 -->

## Function Description

Obtains the workspace size required for model execution based on the model file and the workspace optimization mode.

When the memory is managed by the user, to avoid memory waste, call this API to query the workspace size required for model execution before applying for the workspace. Compared with [aclmdlQuerySize](aclmdlQuerySize.md), this API can specify, through `memOptimizeMode`, whether to query the workspace size after input/output memory optimization.

If the Shape of the model input data is uncertain, this API cannot be called to query the memory size. In this case, the memory cannot be managed by the user when loading the model, so a model loading API that manages memory by the system must be used (for example, [aclmdlLoadFromFile](aclmdlLoadFromFile.md) and [aclmdlLoadFromMem](aclmdlLoadFromMem.md)).

## Prototype

```c
aclError aclmdlQueryWorkspaceSize(const char *fileName, size_t memOptimizeMode, size_t *workSize)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| fileName | Input | Pointer to the model file path, which includes the file name. The user running the program (APP) must have access permission to this path.<br>The model file here is an OM2 model file.<br>For details about how to obtain an OM2 file, see "[--mode](../../../../user_guides/atc_tools/CLI_options/--mode.md)" in *[ATC offline model conversion tool](../../../../user_guides/atc_tools/README.md)*. |
| memOptimizeMode | Input | Workspace optimization mode.<br>Value range: `ACL_WORKSPACE_MEM_OPTIMIZE_DEFAULT` or `ACL_WORKSPACE_MEM_OPTIMIZE_INPUTOUTPUT`.<br>`ACL_WORKSPACE_MEM_OPTIMIZE_DEFAULT` indicates querying the full workspace size required for model execution.<br>`ACL_WORKSPACE_MEM_OPTIMIZE_INPUTOUTPUT` indicates querying the workspace size after input/output memory optimization. |
| workSize | Output | Pointer to the working memory size required for model execution, in bytes.<br>This memory is Device memory and must be allocated and released by the user. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).

## Constraints

- This API supports querying the workspace size of OM2 model files, but does not support querying the workspace size of OM model files. When an OM model file is used to call this API, `ACL_ERROR_API_NOT_SUPPORT` is returned.
- When `memOptimizeMode` is set to `ACL_WORKSPACE_MEM_OPTIMIZE_INPUTOUTPUT`, if the workspace size of the model file has no optimization effect after `ACL_MDL_WORKSPACE_MEM_OPTIMIZE` is set to `ACL_WORKSPACE_MEM_OPTIMIZE_INPUTOUTPUT` through the [aclmdlLoadWithConfig](aclmdlLoadWithConfig.md) API, the query result of `workSize` is the same as that when `memOptimizeMode` is set to `ACL_WORKSPACE_MEM_OPTIMIZE_DEFAULT`.
