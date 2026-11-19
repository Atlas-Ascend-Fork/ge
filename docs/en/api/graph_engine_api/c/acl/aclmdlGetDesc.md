# aclmdlGetDesc

<!-- md-trans-meta sourceCommit=fcba9ddddeed35cd2a7a0ade347fab638d62fcb2 translatedAt=2026-11-17T14:18:29.125Z pushedAt=2026-11-18T03:30:26.877Z nl=2 -->

## Product Support

<!-- npu="950" id162 -->
- Ascend 950PR&950DT products: Supported
<!-- end id162 -->
<!-- npu="A3" id163 -->
- Atlas A3 products: Supported
<!-- end id163 -->
<!-- npu="910b" id164 -->
- Atlas A2 products: Supported
<!-- end id164 -->
<!-- npu="310b" id165 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id165 -->
<!-- npu="310p" id166 -->
- Atlas inference products: Supported
<!-- end id166 -->
<!-- npu="910" id167 -->
- Atlas training products: Supported
<!-- end id167 -->
<!-- npu="IPV350" id168 -->
- IPV350: support
<!-- end id168 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlGetDesc_res.md#id1 -->

## Function Description

Obtains the model description information of a model based on the om/om2 model ID.

## Prototype

```c
aclError aclmdlGetDesc(aclmdlDesc *modelDesc, uint32_t modelId)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| modelDesc | Output | Pointer to the aclmdlDesc type.<br>Call the [aclmdlCreateDesc](aclmdlCreateDesc.md) API in advance to create data of the aclmdlDesc type. |
| modelId | Input | Model ID.<br>After a model is loaded successfully by calling the [aclmdlLoadFromFile](aclmdlLoadFromFile.md) API, [aclmdlLoadFromMem](aclmdlLoadFromMem.md) API, [aclmdlLoadFromFileWithMem](aclmdlLoadFromFileWithMem.md) API, or [aclmdlLoadFromMemWithMem](aclmdlLoadFromMemWithMem.md) API, the model ID is returned. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
