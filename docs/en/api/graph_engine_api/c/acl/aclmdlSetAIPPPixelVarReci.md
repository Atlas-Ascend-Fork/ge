# aclmdlSetAIPPPixelVarReci

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:35:23.914Z pushedAt=2026-10-29T03:36:57.422Z nl=2 -->

## Product Support

<!-- npu="950" id225 -->
- Ascend 950PR&950DT products: Supported
<!-- end id225 -->
<!-- npu="A3" id226 -->
- Atlas A3 products: Supported
<!-- end id226 -->
<!-- npu="910b" id227 -->
- Atlas A2 products: Supported
<!-- end id227 -->
<!-- npu="310b" id228 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id228 -->
<!-- npu="310p" id229 -->
- Atlas inference products: Supported
<!-- end id229 -->
<!-- npu="910" id230 -->
- Atlas training products: Supported
<!-- end id230 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlSetAIPPPixelVarReci_res.md#id1 -->

## Feature Description

Sets the variance of a channel in dynamic AIPP scenarios.

## Prototype

```c
aclError aclmdlSetAIPPPixelVarReci(aclmdlAIPP *aippParmsSet,
float dtcPixelVarReciChn0,
float dtcPixelVarReciChn1,
float dtcPixelVarReciChn2,
float dtcPixelVarReciChn3,
uint64_t batchIndex)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| aippParmsSet | Output | Pointer to the dynamic AIPP parameter object.<br>Call [aclmdlCreateAIPP](aclmdlCreateAIPP.md) in advance to create data of the aclmdlAIPP type. |
| dtcPixelVarReciChn0 | Input | Reciprocal of the variance of channel 0. The default value is 1.0.<br>Value range: [-65504, 65504] |
| dtcPixelVarReciChn1 | Input | Reciprocal of the variance of channel 1. The default value is 1.0.<br>Value range: [-65504, 65504] |
| dtcPixelVarReciChn2 | Input | Reciprocal of the variance of channel 2. The default value is 1.0.<br>Value range: [-65504, 65504] |
| dtcPixelVarReciChn3 | Input | Reciprocal of the variance of channel 3. The default value is 1.0. If there are only three channels, set this parameter to 1.0.<br>Value range: [-65504, 65504] |
| batchIndex | Input | Specifies the batch on which the channel variance is set for the image.<br>Value range: [0, batchSize)<br>batchSize is set when calling [aclmdlCreateAIPP](aclmdlCreateAIPP.md) to create data of the aclmdlAIPP type. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
