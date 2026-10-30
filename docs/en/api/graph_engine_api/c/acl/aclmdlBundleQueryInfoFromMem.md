# aclmdlBundleQueryInfoFromMem

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T14:57:19.748Z pushedAt=2026-10-29T02:56:40.007Z nl=2 -->

## Product Support

<!-- npu="950" id1098 -->
- Ascend 950PR&950DT products: Supported
<!-- end id1098 -->
<!-- npu="A3" id1099 -->
- Atlas A3 products: Supported
<!-- end id1099 -->
<!-- npu="910b" id1100 -->
- Atlas A2 products: Supported
<!-- end id1100 -->
<!-- npu="310b" id1101 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id1101 -->
<!-- npu="310p" id1102 -->
- Atlas inference products: Supported
<!-- end id1102 -->
<!-- npu="910" id1103 -->
- Atlas training products: Supported
<!-- end id1103 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlBundleQueryInfoFromMem_res.md#id1 -->

## Feature Description

During model execution, if variables need to be dynamically updated, you can call this API to obtain model description information from memory.

## Prototype

```c
aclError aclmdlBundleQueryInfoFromMem(const void *model, size_t modelSize, aclmdlBundleQueryInfo *queryInfo)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| model | Input | Pointer to the memory address where the model data is stored.<br>The model file here is built based on the graph building API. Call aclgrphBundleBuildModel to compile the model, call aclgrphBundleSaveModel to save the model, and then read the saved om model file into memory by yourself. For details about the graph building API, see [aclgrphBundleBuildModel](../../cpp/ge/aclgrphBundleBuildModel.md). |
| modelSize | Input | Length of the model data in memory, in bytes. |
| queryInfo | Output | Data of the aclmdlBundleQueryInfo type. Call [aclmdlBundleCreateQueryInfo](aclmdlBundleCreateQueryInfo.md) in advance to create it. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).

## Constraints

<!-- npu="950,A3,910b,910,310p,310b" id1 -->
In Ascend EP mode, the memory for the model parameter must be allocated on the Host.
<!-- end id1 -->

<!-- npu="310b" id2 -->
In Ascend RC mode, the memory for the model parameter must be allocated on the Device.
<!-- end id2 -->

<!-- npu="310p" id3 -->
In Control CPU open mode, the memory for the model parameter must be allocated on the Device.
<!-- end id3 -->

<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlBundleQueryInfoFromMem_res.md#id2 -->
