# aclmdlQuerySizeFromMem

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:30:53.023Z pushedAt=2026-10-29T03:34:06.836Z nl=2 -->

## Product Support

<!-- npu="950" id741 -->
- Ascend 950PR&950DT products: Supported
<!-- end id741 -->
<!-- npu="A3" id742 -->
- Atlas A3 products: Supported
<!-- end id742 -->
<!-- npu="910b" id743 -->
- Atlas A2 products: Supported
<!-- end id743 -->
<!-- npu="310b" id744 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id744 -->
<!-- npu="310p" id745 -->
- Atlas inference products: Supported
<!-- end id745 -->
<!-- npu="910" id746 -->
- Atlas training products: Supported
<!-- end id746 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlQuerySizeFromMem_res.md#id1 -->

## Feature Description

Obtains the weight memory size and working memory size required for model execution based on the model data in memory.

When memory is managed by you, to avoid memory waste, call this API to query the working memory size and weight memory size required for model running before applying for the working memory and weight memory. If the shape of the model input data is not fixed, you cannot call this API to query the memory size, and memory cannot be managed by you when loading the model. In this case, use a model loading API that manages memory by the system, for example, [aclmdlLoadFromFile](aclmdlLoadFromFile.md) or [aclmdlLoadFromMem](aclmdlLoadFromMem.md).

## Prototype

```c
aclError aclmdlQuerySizeFromMem(const void *model, size_t modelSize, size_t *workSize, size_t *weightSize)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| model | Input | Pointer to the model data. |
| modelSize | Input | Length of the model data, in bytes. |
| workSize | Output | Pointer to the working memory size required for model execution, in bytes.<br>This memory is Device memory and must be allocated and released by the user. |
| weightSize | Output | Pointer to the weight memory size required for model execution, in bytes.<br>This memory is Device memory and must be allocated and released by the user. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
