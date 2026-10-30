# aclmdlSetAIPPInputFormat

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:34:23.242Z pushedAt=2026-10-29T03:36:41.946Z nl=2 -->

## Product Support

<!-- npu="950" id622 -->
- Ascend 950PR&950DT products: Supported
<!-- end id622 -->
<!-- npu="A3" id623 -->
- Atlas A3 products: Supported
<!-- end id623 -->
<!-- npu="910b" id624 -->
- Atlas A2 products: Supported
<!-- end id624 -->
<!-- npu="310b" id625 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id625 -->
<!-- npu="310p" id626 -->
- Atlas inference products: Supported
<!-- end id626 -->
<!-- npu="910" id627 -->
- Atlas training products: Supported
<!-- end id627 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlSetAIPPInputFormat_res.md#id1 -->

## Feature Description

In dynamic AIPP scenarios, the format of the raw input image must be set.

## Prototype

```c
aclError aclmdlSetAIPPInputFormat(aclmdlAIPP *aippParmsSet, aclAippInputFormat inputFormat)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| aippParmsSet | Output | Pointer to the dynamic AIPP parameter object.<br>Call [aclmdlCreateAIPP](aclmdlCreateAIPP.md) in advance to create data of the aclmdlAIPP type. |
| inputFormat | Input | Format of the original input image. See [aclAippInputFormat](aclAippInputFormat.md) for the type definition. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
