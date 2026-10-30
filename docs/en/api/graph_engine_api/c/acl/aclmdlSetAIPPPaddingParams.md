# aclmdlSetAIPPPaddingParams

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:35:16.030Z pushedAt=2026-10-29T03:36:50.683Z nl=2 -->

## Product Support

<!-- npu="950" id937 -->
- Ascend 950PR&950DT products: Supported
<!-- end id937 -->
<!-- npu="A3" id938 -->
- Atlas A3 products: Supported
<!-- end id938 -->
<!-- npu="910b" id939 -->
- Atlas A2 products: Supported
<!-- end id939 -->
<!-- npu="310b" id940 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id940 -->
<!-- npu="310p" id941 -->
- Atlas inference products: Supported
<!-- end id941 -->
<!-- npu="910" id942 -->
- Atlas training products: Supported
<!-- end id942 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlSetAIPPPaddingParams_res.md#id1 -->

## Feature Description

Sets parameters related to padding in dynamic AIPP scenarios.

## Prototype

```c
aclError aclmdlSetAIPPPaddingParams(aclmdlAIPP *aippParmsSet, int8_t paddingSwitch,
int32_t paddingSizeTop, int32_t paddingSizeBottom,
int32_t paddingSizeLeft, int32_t paddingSizeRight,
uint64_t batchIndex)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| aippParmsSet | Output | Pointer to the dynamic AIPP parameter object.<br>Call [aclmdlCreateAIPP](aclmdlCreateAIPP.md) in advance to create data of the aclmdlAIPP type. |
| paddingSwitch | Input | Whether to pad the image. Value range:<br><br>  - 0: Do not pad the image. When this parameter is set to 0, the paddingSizeTop, paddingSizeBottom, paddingSizeLeft, and paddingSizeRight parameters are invalid.<br>  - 1: Pad the image. |
| paddingSizeTop | Input | Value to pad at the top of the image.<br>Value range: [0, 32] |
| paddingSizeBottom | Input | Value to pad at the bottom of the image.<br>Value range: [0, 32] |
| paddingSizeLeft | Input | Value to pad on the left of the image.<br>Value range: [0, 32] |
| paddingSizeRight | Input | Value to pad on the right of the image.<br>Value range: [0, 32] |
| batchIndex | Input | Specifies the batch on which the image is padded.<br>Value range: [0,batchSize)<br>batchSize is set when calling [aclmdlCreateAIPP](aclmdlCreateAIPP.md) to create data of the aclmdlAIPP type. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).

## Constraints

After padding, the image width must be less than or equal to 1080.
