# aclmdlDestroyAIPP

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:03:48.020Z pushedAt=2026-10-29T03:00:58.342Z nl=2 -->

## Product Support

<!-- npu="950" id832 -->
- Ascend 950PR&950DT products: Supported
<!-- end id832 -->
<!-- npu="A3" id833 -->
- Atlas A3 products: Supported
<!-- end id833 -->
<!-- npu="910b" id834 -->
- Atlas A2 products: Supported
<!-- end id834 -->
<!-- npu="310b" id835 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id835 -->
<!-- npu="310p" id836 -->
- Atlas inference products: Supported
<!-- end id836 -->
<!-- npu="910" id837 -->
- Atlas training products: Supported
<!-- end id837 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlDestroyAIPP_res.md#id1 -->

## Feature Description

Destroys the aclmdlAIPP data created by the [aclmdlCreateAIPP](aclmdlCreateAIPP.md) API.

## Prototype

```c
aclError aclmdlDestroyAIPP(const aclmdlAIPP *aippParmsSet)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| aippParmsSet | Input | Pointer to the aclmdlAIPP object to be destroyed. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
