# aclmdlQuerySize

<!-- md-trans-meta sourceCommit=aaa82213df16687539020ed5cae4f006363bb63c translatedAt=2026-11-17T14:39:57.447Z pushedAt=2026-11-18T03:35:46.754Z nl=2 -->

## Product Support

<!-- npu="950" id337 -->
- Ascend 950PR&950DT products: Supported
<!-- end id337 -->
<!-- npu="A3" id338 -->
- Atlas A3 products: Supported
<!-- end id338 -->
<!-- npu="910b" id339 -->
- Atlas A2 products: Supported
<!-- end id339 -->
<!-- npu="310b" id340 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id340 -->
<!-- npu="310p" id341 -->
- Atlas inference products: Supported
<!-- end id341 -->
<!-- npu="910" id342 -->
- Atlas training products: Supported
<!-- end id342 -->
<!-- npu="IPV350" id343 -->
- IPV350: support
<!-- end id343 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlQuerySize_res.md#id1 -->

## Function Description

Obtains the weight memory size and working memory size required for model execution based on the OM or OM2 model file.

When memory is managed by you, to avoid memory waste, call this API to query the working memory size and weight memory size required for model running before applying for the working memory and weight memory. If the shape of the model input data is not fixed, you cannot call this API to query the memory size, and memory cannot be managed by you when loading the model. In this case, use a model loading API that manages memory by the system, for example, [aclmdlLoadFromFile](aclmdlLoadFromFile.md) or [aclmdlLoadFromMem](aclmdlLoadFromMem.md).

## Prototype

```c
aclError aclmdlQuerySize(const char *fileName, size_t *workSize, size_t *weightSize)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| fileName | Input | Pointer to the model file path, which includes the file name. The user running the program (APP) must have access permission to this path.<br>The model file here is an OM or OM2 model file.<br>For details about how to obtain an OM or OM2 file, see "[--mode](../../../../user_guides/atc_tools/CLI_options/--mode.md)" in *[ATC offline model conversion tool](../../../../user_guides/atc_tools/README.md)*. |
| workSize | Output | Pointer to the working memory size required for model execution, in bytes.<br>This memory is Device memory and must be allocated and released by the user. |
| weightSize | Output | Pointer to the weight memory size required for model execution, in bytes.<br>This memory is Device memory and must be allocated and released by the user. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
