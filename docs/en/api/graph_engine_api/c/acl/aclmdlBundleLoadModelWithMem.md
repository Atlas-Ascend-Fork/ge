# aclmdlBundleLoadModelWithMem

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T14:56:36.318Z pushedAt=2026-10-29T02:56:20.379Z nl=2 -->

## Product Support

<!-- npu="950" id671 -->
- Ascend 950PR&950DT products: Supported
<!-- end id671 -->
<!-- npu="A3" id672 -->
- Atlas A3 products: Supported
<!-- end id672 -->
<!-- npu="910b" id673 -->
- Atlas A2 products: Supported
<!-- end id673 -->
<!-- npu="310b" id674 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id674 -->
<!-- npu="310p" id675 -->
- Atlas inference products: Supported
<!-- end id675 -->
<!-- npu="910" id676 -->
- Atlas training products: Supported
<!-- end id676 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlBundleLoadModelWithMem_res.md#id1 -->

## Feature Description

Loads a graph from the model by graph index, with the model runtime memory managed by you.

## Prototype

```c
aclError aclmdlBundleLoadModelWithMem(uint32_t bundleId, size_t index, void *workPtr, size_t workSize, void *weightPtr, size_t weightSize, uint32_t *modelId)
```

## Parameters

| Parameter Name | Input/Output | Description |
| --- | --- | --- |
| bundleId | Input | bundleId returned after the model is successfully initialized by calling the [aclmdlBundleInitFromFile](aclmdlBundleInitFromFile.md) or [aclmdlBundleInitFromMem](aclmdlBundleInitFromMem.md) API. |
| index | Input | Index.<br>After you call the [aclmdlBundleGetQueryModelNum](aclmdlBundleGetQueryModelNum.md) API to obtain the total number of graphs, the value range of index is [0, (total number of graphs - 1)]. |
| workPtr | Input | Address pointer to the working memory (for storing temporary data during model execution) required by the model on the Device. This memory is managed by you and must not be released during model execution.<br>If a null pointer is passed to workPtr, the memory is managed by the system.<br>When the working memory is managed by you, if multiple models are executed serially, they can share the same working memory. However, you must ensure the serial execution order of the models, and the working memory size must be allocated based on the largest working memory among the multiple models. For example, ensure serial execution in the following ways:<br>  - For synchronous model execution, use a lock to ensure that execution tasks are serial.<br>  - For asynchronous model execution, use the same stream to ensure that execution tasks are serial. |
| workSize | Input | Size of the working memory required by the model, in bytes. This parameter is invalid when workPtr is a null pointer. |
| weightPtr | Input | Address pointer to the weight memory (for storing weight data) of the model on the Device. This memory is managed by you and must not be released during model execution.<br>If a null pointer is passed to weightPtr, the memory is managed by the system.<br>When the weight memory is managed by you, in a multi-thread scenario, if the same model is loaded once in each thread, the weightPtr can be shared because the weightPtr memory is read-only during inference. Note that weightPtr must not be released while it is being shared. |
| weightSize | Input | Size of the weight memory required by the model, in bytes. This parameter is invalid when weightPtr is a null pointer. |
| modelId | Output | ID of the graph that can actually be executed. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
