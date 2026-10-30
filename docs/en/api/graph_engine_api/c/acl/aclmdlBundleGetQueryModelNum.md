# aclmdlBundleGetQueryModelNum

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T14:52:16.448Z pushedAt=2026-10-29T02:54:37.619Z nl=2 -->

## Product Support

<!-- npu="950" id134 -->
- Ascend 950PR&950DT products: Supported
<!-- end id134 -->
<!-- npu="A3" id135 -->
- Atlas A3 products: Supported
<!-- end id135 -->
<!-- npu="910b" id136 -->
- Atlas A2 products: Supported
<!-- end id136 -->
<!-- npu="310b" id137 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id137 -->
<!-- npu="310p" id138 -->
- Atlas inference products: Supported
<!-- end id138 -->
<!-- npu="910" id139 -->
- Atlas training products: Supported
<!-- end id139 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlBundleGetQueryModelNum_res.md#id1 -->

## Feature Description

Queries the total number of graphs in the model based on aclmdlBundleQueryInfo.

## Prototype

```c
aclError aclmdlBundleGetQueryModelNum(const aclmdlBundleQueryInfo *queryInfo, size_t *modelNum)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| queryInfo | Input | Data of the aclmdlBundleQueryInfo type, which must be created in advance by calling [aclmdlBundleCreateQueryInfo](aclmdlBundleCreateQueryInfo.md). |
| modelNum | Output | Total number of graphs in the model. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
