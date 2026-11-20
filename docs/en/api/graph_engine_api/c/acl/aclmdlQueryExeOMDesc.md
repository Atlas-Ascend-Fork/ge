# aclmdlQueryExeOMDesc

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-11-17T14:38:27.897Z pushedAt=2026-11-18T03:35:30.418Z nl=2 -->

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
<!-- npu="IPV350" id252 -->
- IPV350: support
<!-- end id252 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlQueryExeOMDesc_res.md#id1 -->

## Function Description

Obtains the memory sizes required for model execution, including working memory, weight memory, model description information, and static and dynamic shape tasks, based on the model file.

<!-- npu="IPV350" id1 -->
The model file here is an exeom model file. For details about how to obtain an exeom file, see "[--mode](../../../../user_guides/atc_tools/CLI_options/--mode.md)" in *[ATC Offline Model Compilation Tool](../../../../user_guides/atc_tools/README.md)*.
<!-- end id1 -->

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
