# aclmdlGetNumOutputs

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:20:10.043Z pushedAt=2026-10-29T03:19:14.882Z nl=2 -->

## Product Support

<!-- npu="950" id1014 -->
- Ascend 950PR&950DT products: Supported
<!-- end id1014 -->
<!-- npu="A3" id1015 -->
- Atlas A3 products: Supported
<!-- end id1015 -->
<!-- npu="910b" id1016 -->
- Atlas A2 products: Supported
<!-- end id1016 -->
<!-- npu="310b" id1017 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id1017 -->
<!-- npu="310p" id1018 -->
- Atlas inference products: Supported
<!-- end id1018 -->
<!-- npu="910" id1019 -->
- Atlas training products: Supported
<!-- end id1019 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlGetNumOutputs_res.md#id1 -->

## Feature Description

Obtains the number of outputs of a model based on the model description.

## Prototype

```c
size_t aclmdlGetNumOutputs(aclmdlDesc *modelDesc)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| modelDesc | Input | Pointer to the aclmdlDesc type.<br>Call the [aclmdlCreateDesc](aclmdlCreateDesc.md) API in advance to create the aclmdlDesc type data. |

## Return Value

Number of outputs of the model.
