# aclmdlSetInputAIPP

<!-- md-trans-meta sourceCommit=aaa82213df16687539020ed5cae4f006363bb63c translatedAt=2026-11-17T14:53:47.422Z pushedAt=2026-11-18T03:39:30.800Z nl=2 -->

## Product Support

<!-- npu="950" id1196 -->
- Ascend 950PR&950DT products: Supported
<!-- end id1196 -->
<!-- npu="A3" id1197 -->
- Atlas A3 products: Supported
<!-- end id1197 -->
<!-- npu="910b" id1198 -->
- Atlas A2 products: Supported
<!-- end id1198 -->
<!-- npu="310b" id1199 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id1199 -->
<!-- npu="310p" id1200 -->
- Atlas inference products: Supported
<!-- end id1200 -->
<!-- npu="910" id1201 -->
- Atlas training products: Supported
<!-- end id1201 -->
<!-- npu="IPV350" id1202 -->
- IPV350: not supported
<!-- end id1202 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlSetInputAIPP_res.md#id1 -->

## Function Description

In the dynamic AIPP scenario, sets the AIPP parameter values for om or om2 model inference based on the input index of the specified dynamic AIPP input.

The calculation methods and calculation order of the operations supported by dynamic AIPP are as follows: matting -\> color space conversion -\> mean subtraction/normalization -\>padding.

## Prototype

```c
aclError aclmdlSetInputAIPP(uint32_t modelId, aclmdlDataset *dataset, size_t index, const aclmdlAIPP *aippParmsSet)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| modelId | Input | Model ID.<br>After the model is loaded successfully by calling the [aclmdlLoadFromFile](aclmdlLoadFromFile.md) API, the [aclmdlLoadFromMem](aclmdlLoadFromMem.md) API, the [aclmdlLoadFromFileWithMem](aclmdlLoadFromFileWithMem.md) API, or the [aclmdlLoadFromMemWithMem](aclmdlLoadFromMemWithMem.md) API, the model ID is returned. |
| dataset | Input | Pointer to the input data for model inference. See [aclmdlDataset](aclmdlDataset.md) for the type definition.<br>Use the aclmdlDataset type to describe the input data for model inference. The input memory address and memory size are described by the aclDataBuffer type. |
| index | Input | Input index that identifies the dynamic AIPP input.<br>When there are multiple dynamic AIPP inputs, call [aclmdlGetAippType](aclmdlGetAippType.md) to obtain the input index of the dynamic AIPP input associated with the specified model input.<br>For forward compatibility, if there is only one dynamic AIPP input, call [aclmdlGetInputIndexByName](aclmdlGetInputIndexByName.md) to obtain it. The input name is fixed to ACL_DYNAMIC_AIPP_NAME. |
| aippParmsSet | Input | Pointer to the dynamic AIPP parameter object. See [aclmdlAIPP](aclmdlAIPP.md) for the type definition.<br>Call [aclmdlCreateAIPP](aclmdlCreateAIPP.md) in advance to create data of the aclmdlAIPP type. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).

## Constraints

The width and height of the image processed by dynamic AIPP must be consistent with the width and height in the input shape of the original model.

In multi-batch scenarios, the width and height of the output image after dynamic AIPP are calculated based on the configuration of each batch. The width and height of the output image of each batch after dynamic AIPP must be consistent. The formulas for calculating the width and height of the output image are shown in the following table.

After matting, scaling, or padding, the width and height of the image are validated according to the following rules, where aippOutputW and aippOutputH represent the width and height of the AIPP output image, respectively, and the other parameters are the input parameters of the [aclmdlSetAIPPSrcImageSize](aclmdlSetAIPPSrcImageSize.md), [aclmdlSetAIPPScfParams](aclmdlSetAIPPScfParams.md), [aclmdlSetAIPPCropParams](aclmdlSetAIPPCropParams.md), and [aclmdlSetAIPPPaddingParams](aclmdlSetAIPPPaddingParams.md) APIs.

**Table 1**  Formulas for calculating the width and height of the output image

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
