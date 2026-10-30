# aclmdlGetDesc

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:12:02.277Z pushedAt=2026-10-29T03:11:25.560Z nl=2 -->

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
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlGetDesc_res.md#id1 -->

## Feature Description

Obtains the model description of a model based on the model ID.

## Prototype

```c
aclError aclmdlGetDesc(aclmdlDesc *modelDesc, uint32_t modelId)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| modelDesc | Output | Pointer to the aclmdlDesc type.<br>Call the [aclmdlCreateDesc](aclmdlCreateDesc.md) API in advance to create data of the aclmdlDesc type. |
| modelId | Input | Model ID.<br>After the model is loaded successfully by calling the [aclmdlLoadFromFile](aclmdlLoadFromFile.md) API, [aclmdlLoadFromMem](aclmdlLoadFromMem.md) API, [aclmdlLoadFromFileWithMem](aclmdlLoadFromFileWithMem.md) API, or [aclmdlLoadFromMemWithMem](aclmdlLoadFromMemWithMem.md) API, the model ID is returned. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
