# aclmdlSetAIPPCropParams

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:32:52.080Z pushedAt=2026-10-29T03:36:08.593Z nl=2 -->

## Product Support

<!-- npu="950" id720 -->
- Ascend 950PR&950DT products: Supported
<!-- end id720 -->
<!-- npu="A3" id721 -->
- Atlas A3 products: Supported
<!-- end id721 -->
<!-- npu="910b" id722 -->
- Atlas A2 products: Supported
<!-- end id722 -->
<!-- npu="310b" id723 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id723 -->
<!-- npu="310p" id724 -->
- Atlas inference products: Supported
<!-- end id724 -->
<!-- npu="910" id725 -->
- Atlas training products: Supported
<!-- end id725 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlSetAIPPCropParams_res.md#id1 -->

## Feature Description

Sets crop-related parameters in dynamic AIPP scenarios.

## Prototype

```c
aclError aclmdlSetAIPPCropParams(aclmdlAIPP *aippParmsSet, int8_t cropSwitch,
int32_t cropStartPosW, int32_t cropStartPosH,
int32_t cropSizeW, int32_t cropSizeH,
uint64_t batchIndex)
```

## Parameters

| Parameter Name | Input/Output | Description |
| --- | --- | --- |
| aippParmsSet | Output | Pointer to the dynamic AIPP parameter object.<br>Call [aclmdlCreateAIPP](aclmdlCreateAIPP.md) in advance to create data of the aclmdlAIPP type. |
| cropSwitch | Input | Whether to crop the image. Value Range:<br><br>  - 0: Do not crop the image. When this parameter is set to 0, the cropStartPosW, cropStartPosH, cropSizeW, and cropSizeH parameters are invalid.<br>  - 1: Crop the image. |
| cropStartPosW | Input | Horizontal coordinate of the starting point of the crop area in the image.<br>For images in YUV420SP_U8 format, the value must be an even number.<br>Value Range: [0, 4095] |
| cropStartPosH | Input | Vertical coordinate of the starting point of the crop area in the image.<br>For images in YUV420SP_U8 format, the value must be an even number.<br>Value Range: [0, 4095] |
| cropSizeW | Input | Width of the crop area.<br>Value Range: [1, 4096] |
| cropSizeH | Input | Height of the crop area.<br>Value Range: [1, 4096] |
| batchIndex | Input | Specifies the batch on which the image is cropped.<br>Value Range: [0, batchSize)<br>batchSize is set when calling [aclmdlCreateAIPP](aclmdlCreateAIPP.md) to create data of the aclmdlAIPP type. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).

## Constraints

If the crop function is enabled, the parameters set through the [aclmdlSetAIPPSrcImageSize](aclmdlSetAIPPSrcImageSize.md) API and those set through the aclmdlSetAIPPCropParams API must satisfy the following formulas:

- cropSizeW+cropStartPosW ≤ srcImageSizeW
- cropSizeH+cropStartPosH ≤ srcImageSizeH
