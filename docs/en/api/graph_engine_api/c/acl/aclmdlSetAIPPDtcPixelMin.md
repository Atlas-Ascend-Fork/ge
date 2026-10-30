# aclmdlSetAIPPDtcPixelMin

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:34:27.045Z pushedAt=2026-10-29T03:36:29.504Z nl=2 -->

## Product Support

<!-- npu="950" id1217 -->
- Ascend 950PR&950DT products: Supported
<!-- end id1217 -->
<!-- npu="A3" id1218 -->
- Atlas A3 products: Supported
<!-- end id1218 -->
<!-- npu="910b" id1219 -->
- Atlas A2 products: Supported
<!-- end id1219 -->
<!-- npu="310b" id1220 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id1220 -->
<!-- npu="310p" id1221 -->
- Atlas inference products: Supported
<!-- end id1221 -->
<!-- npu="910" id1222 -->
- Atlas training products: Supported
<!-- end id1222 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlSetAIPPDtcPixelMin_res.md#id1 -->

## Feature Description

Sets the minimum value of a channel in dynamic AIPP scenarios.

## Prototype

```c
aclError aclmdlSetAIPPDtcPixelMin(aclmdlAIPP *aippParmsSet,
float dtcPixelMinChn0,
float dtcPixelMinChn1,
float dtcPixelMinChn2,
float dtcPixelMinChn3,
uint64_t batchIndex)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| aippParmsSet | Output | Pointer to the dynamic AIPP parameter object.<br>Call [aclmdlCreateAIPP](aclmdlCreateAIPP.md) in advance to create data of the aclmdlAIPP type. |
| dtcPixelMinChn0 | Input | Minimum value of channel 0.<br>Value range: [0, 255] |
| dtcPixelMinChn1 | Input | Minimum value of channel 1.<br>Value range: [0, 255] |
| dtcPixelMinChn2 | Input | Minimum value of channel 2.<br>Value range: [0, 255] |
| dtcPixelMinChn3 | Input | Minimum value of channel 3. If there are only three channels, set this parameter to 0.<br>Value range: [0, 255] |
| batchIndex | Input | Specifies the batch on which the image channel minimum value is set.<br>Value range: [0,batchSize)<br>batchSize is set when [aclmdlCreateAIPP](aclmdlCreateAIPP.md) is called to create data of the aclmdlAIPP type. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
