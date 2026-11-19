# aclmdlBundleGetQueryModelNum

<!-- md-trans-meta sourceCommit=aaa82213df16687539020ed5cae4f006363bb63c translatedAt=2026-11-17T13:55:14.351Z pushedAt=2026-11-18T03:17:30.225Z nl=2 -->

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
<!-- npu="IPV350" id140 -->
- IPV350: not supported
<!-- end id140 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlBundleGetQueryModelNum_res.md#id1 -->

## Function Description

Queries the total number of graphs in the om/om2 model based on aclmdlBundleQueryInfo.

## Prototype

```c
aclError aclmdlBundleGetQueryModelNum(const aclmdlBundleQueryInfo *queryInfo, size_t *modelNum)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| queryInfo | Input | Data of the aclmdlBundleQueryInfo type. Call [aclmdlBundleCreateQueryInfo](aclmdlBundleCreateQueryInfo.md) in advance to create it. |
| modelNum | Output | Total number of graphs in the model. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
