# aclmdlDestroyConfigHandle

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:03:49.053Z pushedAt=2026-10-29T03:01:06.139Z nl=2 -->

## Product Support

<!-- npu="950" id1028 -->
- Ascend 950PR&950DT products: Supported
<!-- end id1028 -->
<!-- npu="A3" id1029 -->
- Atlas A3 products: Supported
<!-- end id1029 -->
<!-- npu="910b" id1030 -->
- Atlas A2 products: Supported
<!-- end id1030 -->
<!-- npu="310b" id1031 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id1031 -->
<!-- npu="310p" id1032 -->
- Atlas inference products: Supported
<!-- end id1032 -->
<!-- npu="910" id1033 -->
- Atlas training products: Supported
<!-- end id1033 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlDestroyConfigHandle_res.md#id1 -->

## Feature Description

Destroys the aclmdlConfigHandle data created by the [aclmdlCreateConfigHandle](aclmdlCreateConfigHandle.md) API.

## Prototype

```c
aclError aclmdlDestroyConfigHandle(aclmdlConfigHandle *handle)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| handle | Input | Pointer to the aclmdlConfigHandle to be destroyed. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
