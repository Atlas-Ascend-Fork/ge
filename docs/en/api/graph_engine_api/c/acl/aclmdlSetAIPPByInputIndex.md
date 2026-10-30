# aclmdlSetAIPPByInputIndex

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:32:04.286Z pushedAt=2026-10-29T03:36:00.877Z nl=2 -->

## Product Support

<!-- npu="950" id916 -->
- Ascend 950PR&950DT products: Supported
<!-- end id916 -->
<!-- npu="A3" id917 -->
- Atlas A3 products: Supported
<!-- end id917 -->
<!-- npu="910b" id918 -->
- Atlas A2 products: Supported
<!-- end id918 -->
<!-- npu="310b" id919 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id919 -->
<!-- npu="310p" id920 -->
- Atlas inference products: Supported
<!-- end id920 -->
<!-- npu="910" id921 -->
- Atlas training products: Supported
<!-- end id921 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlSetAIPPByInputIndex_res.md#id1 -->

## Feature Description

In a dynamic AIPP scenario, sets the AIPP parameter values for model inference based on the input index of the specified model input.

The computation methods and computation order of the operations supported by dynamic AIPP are as follows: matting -\> color space conversion -\> mean subtraction/normalization -\>padding.

## Prototype

```c
aclError aclmdlSetAIPPByInputIndex(uint32_t modelId, aclmdlDataset *dataset, size_t index, const aclmdlAIPP *aippParmsSet)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| modelId | Input | Model ID.<br>After the model is loaded successfully by calling the [aclmdlLoadFromFile](aclmdlLoadFromFile.md) API, the [aclmdlLoadFromMem](aclmdlLoadFromMem.md) API, the [aclmdlLoadFromFileWithMem](aclmdlLoadFromFileWithMem.md) API, or the [aclmdlLoadFromMemWithMem](aclmdlLoadFromMemWithMem.md) API, the model ID is returned. |
| dataset | Input | Pointer to the input data for model inference. See [aclmdlDataset](aclmdlDataset.md) for the type definition.<br>Data of the aclmdlDataset type is used to describe the input data for model inference. The input memory address and memory size are described by data of the aclDataBuffer type. |
| index | Input | Specifies the input on which dynamic AIPP parameters are set. If the input is not associated with a dynamic AIPP input, an error is returned.<br>Call [aclmdlGetAippType](aclmdlGetAippType.md) to query whether the specified input of the specified model has an associated dynamic AIPP input. If yes, the index parameter value of this API must be consistent with the index parameter value of the [aclmdlGetAippType](aclmdlGetAippType.md) API. If no, dynamic AIPP parameters do not need to be set. |
| aippParmsSet | Input | Pointer to the dynamic AIPP parameter object. See [aclmdlAIPP](aclmdlAIPP.md) for the type definition.<br>Call the [aclmdlCreateAIPP](aclmdlCreateAIPP.md) API in advance to create data of the aclmdlAIPP type. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).

## Constraints

The width and height of the image processed by dynamic AIPP must be consistent with the width and height in the input shape of the original model.

In a multi-batch scenario, the width and height of the output image after dynamic AIPP are calculated based on the configuration of each batch. The width and height of the output image of each batch after dynamic AIPP must be consistent. The formulas for calculating the output image width and height are shown in the following table.

After matting, scaling, or padding, the width and height of the image are validated according to the following rules, where aippOutputW and aippOutputH represent the width and height of the AIPP output image, respectively, and the other parameters are the input parameters of the [aclmdlSetAIPPSrcImageSize](aclmdlSetAIPPSrcImageSize.md), [aclmdlSetAIPPScfParams](aclmdlSetAIPPScfParams.md), [aclmdlSetAIPPCropParams](aclmdlSetAIPPCropParams.md), and [aclmdlSetAIPPPaddingParams](aclmdlSetAIPPPaddingParams.md) APIs.

**Table 1**  Formulas for calculating the output image width and height

| Matting | Scaling | Padding | Width and height of the dynamic AIPP output image |
| --- | --- | --- | --- |
| No | No | No | aippOutputW=srcImageSizeW, aippOutputH=srcImageSizeH |
| Yes | No | No | aippOutputW=cropSizeW, aippOutputH=cropSizeH |
| Yes | Yes | No | aippOutputW=scfOutputSizeW, aippOutputH=scfOutputSizeH |
| Yes | No | Yes | aippOutputW=cropSizeW + paddingSizeLeft + paddingSizeRight, aippOutputH=cropSizeH + paddingSizeTop + paddingSizeBottom |
| No | No | Yes | aippOutputW=srcImageSizeW + paddingSizeLeft + paddingSizeRight, aippOutputH=srcImageSizeH + paddingSizeTop + paddingSizeBottom |
| No | Yes | Yes | aippOutputW=scfOutputSizeW + paddingSizeLeft + paddingSizeRight, aippOutputH=scfOutputSizeH + paddingSizeTop + paddingSizeBottom |
| No | Yes | No | aippOutputW=scfOutputSizeW, aippOutputH=scfOutputSizeH |
| Yes | Yes | Yes | aippOutputW=scfOutputSizeW + paddingSizeLeft + paddingSizeRight, aippOutputH=scfOutputSizeH + paddingSizeTop + paddingSizeBottom |
