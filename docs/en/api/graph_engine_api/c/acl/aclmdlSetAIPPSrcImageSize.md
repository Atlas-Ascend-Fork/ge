# aclmdlSetAIPPSrcImageSize

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:37:03.929Z pushedAt=2026-10-29T03:37:45.221Z nl=2 -->

## Product Support

<!-- npu="950" id1119 -->
- Ascend 950PR&950DT products: Supported
<!-- end id1119 -->
<!-- npu="A3" id1120 -->
- Atlas A3 products: Supported
<!-- end id1120 -->
<!-- npu="910b" id1121 -->
- Atlas A2 products: Supported
<!-- end id1121 -->
<!-- npu="310b" id1122 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id1122 -->
<!-- npu="310p" id1123 -->
- Atlas inference products: Supported
<!-- end id1123 -->
<!-- npu="910" id1124 -->
- Atlas training products: Supported
<!-- end id1124 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlSetAIPPSrcImageSize_res.md#id1 -->

## Feature Description

In dynamic AIPP scenarios, you must set the width and height of the original image.

## Prototype

```c
aclError aclmdlSetAIPPSrcImageSize(aclmdlAIPP *aippParmsSet, int32_t srcImageSizeW, int32_t srcImageSizeH)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| aippParmsSet | Output | Pointer to the dynamic AIPP parameter object.<br>Call [aclmdlCreateAIPP](aclmdlCreateAIPP.md) in advance to create data of the aclmdlAIPP type. |
| srcImageSizeW | Input | Width of the original image. For images of the YUV420SP_U8, YUV422SP_U8, or YUYV_U8 type, srcImageSizeW must be an even number.<br>Value range: [2, 4096] |
| srcImageSizeH | Input | Height of the original image. For images of the YUV420SP_U8 type, srcImageSizeH must be an even number.<br>Value range: [1, 4096] |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
