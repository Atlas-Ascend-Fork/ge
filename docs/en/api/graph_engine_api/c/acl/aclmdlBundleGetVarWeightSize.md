# aclmdlBundleGetVarWeightSize

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T14:53:04.610Z pushedAt=2026-10-29T02:54:56.102Z nl=2 -->

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
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlBundleGetVarWeightSize_res.md#id1 -->

## Feature Description

Obtains the memory size of the updatable weights of a model.

## Prototype

```c
aclError aclmdlBundleGetVarWeightSize(const aclmdlBundleQueryInfo *queryInfo, size_t *variableWeightSize)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| queryInfo | Input | Call [aclmdlBundleCreateQueryInfo](aclmdlBundleCreateQueryInfo.md) in advance to create data of the aclmdlBundleQueryInfo type. |
| variableWeightSize | Output | Obtains the memory size of the model's updatable weights. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
