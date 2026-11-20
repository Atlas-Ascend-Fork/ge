# aclmdlBundleGetVarWeightSize

<!-- md-trans-meta sourceCommit=aaa82213df16687539020ed5cae4f006363bb63c translatedAt=2026-11-17T13:56:07.044Z pushedAt=2026-11-18T03:17:43.356Z nl=2 -->

## Product Support

<!-- npu="950" id1056 -->
- Ascend 950PR&950DT products: Supported
<!-- end id1056 -->
<!-- npu="A3" id1057 -->
- Atlas A3 products: Supported
<!-- end id1057 -->
<!-- npu="910b" id1058 -->
- Atlas A2 products: Supported
<!-- end id1058 -->
<!-- npu="310b" id1059 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id1059 -->
<!-- npu="310p" id1060 -->
- Atlas inference products: Supported
<!-- end id1060 -->
<!-- npu="910" id1061 -->
- Atlas training products: Supported
<!-- end id1061 -->
<!-- npu="IPV350" id1062 -->
- IPV350: not supported
<!-- end id1062 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlBundleGetVarWeightSize_res.md#id1 -->

## Function Description

Obtains the size of the model's updatable weight memory for an om/om2 model.

## Prototype

```c
aclError aclmdlBundleGetVarWeightSize(const aclmdlBundleQueryInfo *queryInfo, size_t *variableWeightSize)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| queryInfo | Input | Data of the aclmdlBundleQueryInfo type. Call [aclmdlBundleCreateQueryInfo](aclmdlBundleCreateQueryInfo.md) in advance to create it. |
| variableWeightSize | Output | Obtains the memory size of the updatable weights of a model. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
