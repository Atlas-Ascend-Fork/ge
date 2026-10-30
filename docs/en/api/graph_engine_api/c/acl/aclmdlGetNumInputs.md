# aclmdlGetNumInputs

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:19:59.112Z pushedAt=2026-10-29T03:19:08.724Z nl=2 -->

## Product Support

<!-- npu="950" id211 -->
- Ascend 950PR&950DT products: Supported
<!-- end id211 -->
<!-- npu="A3" id212 -->
- Atlas A3 products: Supported
<!-- end id212 -->
<!-- npu="910b" id213 -->
- Atlas A2 products: Supported
<!-- end id213 -->
<!-- npu="310b" id214 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id214 -->
<!-- npu="310p" id215 -->
- Atlas inference products: Supported
<!-- end id215 -->
<!-- npu="910" id216 -->
- Atlas training products: Supported
<!-- end id216 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlGetNumInputs_res.md#id1 -->

## Feature Description

Obtains the number of inputs of a model based on the model description.

## Prototype

```c
size_t aclmdlGetNumInputs(aclmdlDesc *modelDesc)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| modelDesc | Input | Pointer to the aclmdlDesc type.<br>Call [aclmdlCreateDesc](aclmdlCreateDesc.md) in advance to create data of the aclmdlDesc type. |

## Return Value

Returns the number of model inputs.
