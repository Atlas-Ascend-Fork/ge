# aclmdlUnload

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:41:54.502Z pushedAt=2026-10-29T03:41:51.149Z nl=2 -->

## Product Support

<!-- npu="950" id1042 -->
- Ascend 950PR&950DT products: Supported
<!-- end id1042 -->
<!-- npu="A3" id1043 -->
- Atlas A3 products: Supported
<!-- end id1043 -->
<!-- npu="910b" id1044 -->
- Atlas A2 products: Supported
<!-- end id1044 -->
<!-- npu="310b" id1045 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id1045 -->
<!-- npu="310p" id1046 -->
- Atlas inference products: Supported
<!-- end id1046 -->
<!-- npu="910" id1047 -->
- Atlas training products: Supported
<!-- end id1047 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlUnload_res.md#id1 -->

## Feature Description

After model inference is complete, you can call this API to unload the model and release resources. However, ensure that no other API is currently using the model.

Model loading, model execution, and model unloading must be performed in the same Context. For details about creating a Context, see aclrtCreateContext.

## Prototype

```c
aclError aclmdlUnload(uint32_t modelId)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| modelId | Input | ID of the model to be unloaded. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
