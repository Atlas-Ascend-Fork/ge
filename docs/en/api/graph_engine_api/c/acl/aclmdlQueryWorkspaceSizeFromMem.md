# aclmdlQueryWorkspaceSizeFromMem

<!-- md-trans-meta sourceCommit=952f341a9689e98183fe9174c116ff95ab991801 translatedAt=2026-11-17T14:42:06.425Z pushedAt=2026-11-18T03:36:54.274Z nl=2 -->

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
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlQueryWorkspaceSizeFromMem_res.md#id1 -->

## Function Description

Obtains the workspace size required for model execution based on the model data in memory and the workspace optimization mode.

When memory is managed by the user, to avoid memory waste, call this API to query the workspace size required for model execution before applying for the workspace. Compared with [aclmdlQuerySizeFromMem](aclmdlQuerySizeFromMem.md), this API can use `memOptimizeMode` to specify whether to query the workspace size after input/output memory optimization.

If the Shape of the model input data is uncertain, this API cannot be called to query the memory size. In this case, memory cannot be managed by the user when loading the model, so a model loading API with memory managed by the system must be selected (for example, [aclmdlLoadFromFile](aclmdlLoadFromFile.md) and [aclmdlLoadFromMem](aclmdlLoadFromMem.md)).

## Prototype

```c
aclError aclmdlQueryWorkspaceSizeFromMem(const void *model, size_t modelSize, size_t memOptimizeMode, size_t *workSize)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| model | Input | Pointer to the model data. |
| modelSize | Input | Length of the model data, in bytes. |
| memOptimizeMode | Input | Workspace optimization mode.<br>Value range: `ACL_WORKSPACE_MEM_OPTIMIZE_DEFAULT` or `ACL_WORKSPACE_MEM_OPTIMIZE_INPUTOUTPUT`.<br>`ACL_WORKSPACE_MEM_OPTIMIZE_DEFAULT` indicates querying the full workspace size required for model execution.<br>`ACL_WORKSPACE_MEM_OPTIMIZE_INPUTOUTPUT` indicates querying the workspace size after input/output memory optimization. |
| workSize | Output | Pointer to the working memory size required for model execution, in bytes.<br>This memory is Device memory and must be allocated and released by the user. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).

## Constraints

- This API supports querying the workspace size of OM2 model data, but does not support querying the workspace size of OM model data. When OM model data is used to call this API, `ACL_ERROR_API_NOT_SUPPORT` is returned.
- When `memOptimizeMode` is set to `ACL_WORKSPACE_MEM_OPTIMIZE_INPUTOUTPUT`, if the workspace size of the model data has no optimization effect after `ACL_MDL_WORKSPACE_MEM_OPTIMIZE` is set to `ACL_WORKSPACE_MEM_OPTIMIZE_INPUTOUTPUT` through the [aclmdlLoadWithConfig](aclmdlLoadWithConfig.md) API, the query result of `workSize` is the same as that when `memOptimizeMode` is set to `ACL_WORKSPACE_MEM_OPTIMIZE_DEFAULT`.
