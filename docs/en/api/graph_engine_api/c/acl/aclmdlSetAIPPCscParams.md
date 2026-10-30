# aclmdlSetAIPPCscParams

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:33:24.846Z pushedAt=2026-10-29T03:36:16.867Z nl=2 -->

## Product Support

<!-- npu="950" id902 -->
- Ascend 950PR&950DT products: Supported
<!-- end id902 -->
<!-- npu="A3" id903 -->
- Atlas A3 products: Supported
<!-- end id903 -->
<!-- npu="910b" id904 -->
- Atlas A2 products: Supported
<!-- end id904 -->
<!-- npu="310b" id905 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id905 -->
<!-- npu="310p" id906 -->
- Atlas inference products: Supported
<!-- end id906 -->
<!-- npu="910" id907 -->
- Atlas training products: Supported
<!-- end id907 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlSetAIPPCscParams_res.md#id1 -->

## Feature Description

In dynamic AIPP scenarios, this API sets parameters related to CSC color gamut conversion. If the color gamut conversion switch is disabled, calling this API to set the following parameters has no effect.

```bash
YUV to BGR:
| B |   | cscMatrixR0C0 cscMatrixR0C1 cscMatrixR0C2 | | Y - cscInputBiasR0 |
| G | = | cscMatrixR1C0 cscMatrixR1C1 cscMatrixR1C2 | | U - cscInputBiasR1 | >> 8
| R |   | cscMatrixR2C0 cscMatrixR2C1 cscMatrixR2C2 | | V - cscInputBiasR2 |
BGR to YUV:
| Y |   | cscMatrixR0C0 cscMatrixR0C1 cscMatrixR0C2 | | B |        | cscOutputBiasR0 |
| U | = | cscMatrixR1C0 cscMatrixR1C1 cscMatrixR1C2 | | G | >> 8 + | cscOutputBiasR1 |
| V |   | cscMatrixR2C0 cscMatrixR2C1 cscMatrixR2C2 | | R |        | cscOutputBiasR2 |
```

The color gamut conversion parameter values are strongly related to the format of the image before conversion and the format of the image after conversion. You can refer to the "Advanced Features \> Enabling AIPP \> Color Gamut Conversion Configuration" section in *[ATC Offline Model Compilation Tool](../../../../user_guides/atc_tools/README.md)* to configure the color gamut conversion parameters based on the image format before conversion and the image format after conversion. If the image formats listed in the manual do not meet your requirements, you need to configure the color gamut conversion parameters based on your actual needs.

## Prototype

```c
aclError aclmdlSetAIPPCscParams(aclmdlAIPP *aippParmsSet, int8_t cscSwitch,
int16_t cscMatrixR0C0, int16_t cscMatrixR0C1, int16_t cscMatrixR0C2,
int16_t cscMatrixR1C0, int16_t cscMatrixR1C1,int16_t cscMatrixR1C2,
int16_t cscMatrixR2C0, int16_t cscMatrixR2C1, int16_t cscMatrixR2C2,
uint8_t cscOutputBiasR0, uint8_t cscOutputBiasR1, uint8_t cscOutputBiasR2,
uint8_t cscInputBiasR0, uint8_t cscInputBiasR1, uint8_t cscInputBiasR2)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| aippParmsSet | Output | Pointer to the dynamic AIPP parameter object.<br>Call [aclmdlCreateAIPP](aclmdlCreateAIPP.md) in advance to create data of the aclmdlAIPP type. |
| cscSwitch | Input | Color gamut conversion switch. Value range:<br><br>  - 0: Disables the color gamut conversion switch. When set to 0, the following parameters are invalid.<br>  - 1: Enables the color gamut conversion switch. |
| cscMatrixR0C0 | Input | Color gamut conversion matrix parameter.<br>Value range: [-32677 ,32676] |
| cscMatrixR0C1 | Input | Color gamut conversion matrix parameter.<br>Value range: [-32677 ,32676] |
| cscMatrixR0C2 | Input | Color gamut conversion matrix parameter.<br>Value range: [-32677 ,32676] |
| cscMatrixR1C0 | Input | Color gamut conversion matrix parameter.<br>Value range: [-32677 ,32676] |
| cscMatrixR1C1 | Input | Color gamut conversion matrix parameter.<br>Value range: [-32677 ,32676] |
| cscMatrixR1C2 | Input | Color gamut conversion matrix parameter.<br>Value range: [-32677 ,32676] |
| cscMatrixR2C0 | Input | Color gamut conversion matrix parameter.<br>Value range: [-32677 ,32676] |
| cscMatrixR2C1 | Input | Color gamut conversion matrix parameter.<br>Value range: [-32677 ,32676] |
| cscMatrixR2C2 | Input | Color gamut conversion matrix parameter.<br>Value range: [-32677 ,32676] |
| cscOutputBiasR0 | Input | Output bias for RGB-to-YUV conversion.<br>Value range: [0, 255] |
| cscOutputBiasR1 | Input | Output bias for RGB-to-YUV conversion.<br>Value range: [0, 255] |
| cscOutputBiasR2 | Input | Output bias for RGB-to-YUV conversion.<br>Value range: [0, 255] |
| cscInputBiasR0 | Input | Input bias for YUV-to-RGB conversion.<br>Value range: [0, 255] |
| cscInputBiasR1 | Input | Input bias for YUV-to-RGB conversion.<br>Value range: [0, 255] |
| cscInputBiasR2 | Input | Input bias for YUV-to-RGB conversion.<br>Value range: [0, 255] |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).

## Constraints

If the original image format set through the [aclmdlSetAIPPInputFormat](aclmdlSetAIPPInputFormat.md) API is YUV400, setting color gamut conversion parameters through this API is not supported.
