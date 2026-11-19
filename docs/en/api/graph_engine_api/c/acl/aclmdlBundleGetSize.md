# aclmdlBundleGetSize

<!-- md-trans-meta sourceCommit=aaa82213df16687539020ed5cae4f006363bb63c translatedAt=2026-11-17T13:56:03.587Z pushedAt=2026-11-18T03:17:36.440Z nl=2 -->

## Product Support

<!-- npu="950" id888 -->
- Ascend 950PR&950DT products: Supported
<!-- end id888 -->
<!-- npu="A3" id889 -->
- Atlas A3 products: Supported
<!-- end id889 -->
<!-- npu="910b" id890 -->
- Atlas A2 products: Supported
<!-- end id890 -->
<!-- npu="310b" id891 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id891 -->
<!-- npu="310p" id892 -->
- Atlas inference products: Supported
<!-- end id892 -->
<!-- npu="910" id893 -->
- Atlas training products: Supported
<!-- end id893 -->
<!-- npu="IPV350" id894 -->
- IPV350: not supported
<!-- end id894 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlBundleGetSize_res.md#id1 -->

## Function Description

Obtains the weight memory size and working memory size required for graph execution based on the om/om2 model description information and graph index.

## Prototype

```c
aclError aclmdlBundleGetSize(const aclmdlBundleQueryInfo *queryInfo, size_t index, size_t *workSize, size_t *constWeightSize)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| queryInfo | Input | Data of the aclmdlBundleQueryInfo type. Call [aclmdlBundleCreateQueryInfo](aclmdlBundleCreateQueryInfo.md) in advance to create it. |
| index | Input | Index.<br>After you call the [aclmdlBundleGetQueryModelNum](aclmdlBundleGetQueryModelNum.md) API to obtain the total number of graphs in the model, the value range of this index is [0, (total number of graphs - 1)]. |
| workSize | Output | Pointer to the working memory size required for graph execution, in bytes.<br>This memory is Device memory and must be allocated and released by you. |
| weightSize | Output | Pointer to the weight memory size required for graph execution, in bytes.<br>This memory is Device memory and must be allocated and released by you. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
