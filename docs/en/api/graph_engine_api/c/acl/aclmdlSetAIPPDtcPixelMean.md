# aclmdlSetAIPPDtcPixelMean

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:33:37.859Z pushedAt=2026-10-29T03:36:23.334Z nl=2 -->

## Product Support

<!-- npu="950" id790 -->
- Ascend 950PR&950DT products: Supported
<!-- end id790 -->
<!-- npu="A3" id791 -->
- Atlas A3 products: Supported
<!-- end id791 -->
<!-- npu="910b" id792 -->
- Atlas A2 products: Supported
<!-- end id792 -->
<!-- npu="310b" id793 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id793 -->
<!-- npu="310p" id794 -->
- Atlas inference products: Supported
<!-- end id794 -->
<!-- npu="910" id795 -->
- Atlas training products: Supported
<!-- end id795 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlSetAIPPDtcPixelMean_res.md#id1 -->

## Feature Description

Sets the mean value of a channel in dynamic AIPP scenarios.

## Prototype

```c
aclError aclmdlSetAIPPDtcPixelMean(aclmdlAIPP *aippParmsSet,
int16_t dtcPixelMeanChn0,
int16_t dtcPixelMeanChn1,
int16_t dtcPixelMeanChn2,
int16_t dtcPixelMeanChn3,
uint64_t batchIndex)
```

## Parameters

| Parameter Name | Input/Output | Description |
| --- | --- | --- |
| aippParmsSet | Output | Pointer to the dynamic AIPP parameter object.<br>Call [aclmdlCreateAIPP](aclmdlCreateAIPP.md) in advance to create data of the aclmdlAIPP type. |
| dtcPixelMeanChn0 | Input | Mean value of channel 0.<br>Value Range: [0, 255] |
| dtcPixelMeanChn1 | Input | Mean value of channel 1.<br>Value Range: [0, 255] |
| dtcPixelMeanChn2 | Input | Mean value of channel 2.<br>Value Range: [0, 255] |
| dtcPixelMeanChn3 | Input | Mean value of channel 3.<br>If there are only 3 channels, set this parameter to 0.<br>Value Range: [0, 255] |
| batchIndex | Input | Specifies the batch on which to set the channel mean value for the image.<br>Value Range: [0,batchSize)<br>batchSize is set when calling [aclmdlCreateAIPP](aclmdlCreateAIPP.md) to create data of the aclmdlAIPP type. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
