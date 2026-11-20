# aclmdlGetNumInputs

<!-- md-trans-meta sourceCommit=fcba9ddddeed35cd2a7a0ade347fab638d62fcb2 translatedAt=2026-11-17T14:27:27.513Z pushedAt=2026-11-18T03:32:21.751Z nl=2 -->

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
<!-- npu="IPV350" id217 -->
- IPV350: support
<!-- end id217 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlGetNumInputs_res.md#id1 -->

## Function Description

Obtains the number of model inputs based on the om/om2 model description information.

## Prototype

```c
size_t aclmdlGetNumInputs(aclmdlDesc *modelDesc)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| modelDesc | Input | Pointer to the aclmdlDesc type.<br>Call the [aclmdlCreateDesc](aclmdlCreateDesc.md) API in advance to create data of the aclmdlDesc type. |

## Return Value

Returns the number of model inputs.
