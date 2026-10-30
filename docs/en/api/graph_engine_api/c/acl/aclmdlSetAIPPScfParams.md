# aclmdlSetAIPPScfParams

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:36:18.740Z pushedAt=2026-10-29T03:37:36.918Z nl=2 -->

## Product Support

<!-- npu="950" id15 -->
- Ascend 950PR&950DT products: Supported
<!-- end id15 -->
<!-- npu="A3" id16 -->
- Atlas A3 products: Supported
<!-- end id16 -->
<!-- npu="910b" id17 -->
- Atlas A2 products: Supported
<!-- end id17 -->
<!-- npu="310b" id18 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id18 -->
<!-- npu="310p" id19 -->
- Atlas inference products: Supported
<!-- end id19 -->
<!-- npu="910" id20 -->
- Atlas training products: Supported
<!-- end id20 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlSetAIPPScfParams_res.md#id1 -->

## Feature Description

Sets scaling-related parameters in dynamic AIPP scenarios.

## Prototype

```c
aclError aclmdlSetAIPPScfParams(aclmdlAIPP *aippParmsSet, int8_t scfSwitch,
int32_t scfInputSizeW, int32_t scfInputSizeH,
int32_t scfOutputSizeW, int32_t scfOutputSizeH,
uint64_t batchIndex)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| aippParmsSet | Output | Pointer to the dynamic AIPP parameter object.<br>Call [aclmdlCreateAIPP](aclmdlCreateAIPP.md) in advance to create data of the aclmdlAIPP type. |
| scfSwitch | Input | Whether to perform scaling on the image. Value range:<br><br>  - 0: Do not perform scaling. When set to 0, the scfInputSizeW, scfInputSizeH, scfOutputSizeW, and scfOutputSizeH parameters are invalid.<br>  - 1: Perform scaling. |
| scfInputSizeW | Input | Width of the image before scaling.<br>Value range: [16,4096]<br>If the matting function is enabled, the width of the image before scaling must remain consistent with the [width of the matting area](aclmdlSetAIPPCropParams.md). If the matting function is not enabled, the width of the image before scaling must remain consistent with the [width of the original image](aclmdlSetAIPPSrcImageSize.md). |
| scfInputSizeH | Input | Height of the image before scaling.<br>Value range: [16,4096]<br>If the matting function is enabled, the height of the image before scaling must remain consistent with the [height of the matting area](aclmdlSetAIPPCropParams.md). If the matting function is not enabled, the height of the image before scaling must remain consistent with the [height of the original image](aclmdlSetAIPPSrcImageSize.md). |
| scfOutputSizeW | Input | Width of the image after scaling.<br>Value range: [16,1920] |
| scfOutputSizeH | Input | Height of the image after scaling.<br>Value range: [16,4096] |
| batchIndex | Input | Specifies the batch on which to perform scaling on the image.<br>Value range: [0,batchSize)<br>batchSize is set when calling [aclmdlCreateAIPP](aclmdlCreateAIPP.md) to create data of the aclmdlAIPP type. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).

## Constraints

Scaling ratio scfOutputSizeW/scfInputSizeW∈\[1/16,16\], scfOutputSizeH/scfInputSizeH∈\[1/16,16\].
