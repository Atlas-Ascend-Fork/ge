# aclmdlQueryExeOMDesc

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:29:50.653Z pushedAt=2026-10-29T03:32:06.788Z nl=2 -->

## Product Support

<!-- npu="950" id246 -->
- Ascend 950PR&950DT products: Not supported
<!-- end id246 -->
<!-- npu="A3" id247 -->
- Atlas A3 products: Not supported
<!-- end id247 -->
<!-- npu="910b" id248 -->
- Atlas A2 products: Not supported
<!-- end id248 -->
<!-- npu="310b" id249 -->
- Atlas 200I/500 A2 inference product: Not supported
<!-- end id249 -->
<!-- npu="310p" id250 -->
- Atlas inference products: Not supported
<!-- end id250 -->
<!-- npu="910" id251 -->
- Atlas training products: Not supported
<!-- end id251 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlQueryExeOMDesc_res.md#id1 -->

## Feature Description

Obtains the memory sizes required for model execution, including working memory, weight memory, model description information, and static and dynamic shape tasks, based on the model file.

## Prototype

```c
aclError aclmdlQueryExeOMDesc(const char *fileName, [aclmdlExeOMDesc](aclmdlExeOMDesc.md) *mdlPartitionSize)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| fileName | Input | Pointer to the model file path, including the file name. The user running the application (APP) must have access permission to this path. |
| mdlPartitionSize | Output | Pointer to the structure that stores the size of each partition required for model execution, in bytes. See [aclmdlExeOMDesc](aclmdlExeOMDesc.md) for the type definition. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
