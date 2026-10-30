# aclAippInfo

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-27T14:28:21.387Z pushedAt=2026-10-29T02:02:41.149Z nl=2 -->

```c
typedef struct aclAippInfo {
    aclAippInputFormat inputFormat;
    int32_t srcImageSizeW;
    int32_t srcImageSizeH;
    int8_t cropSwitch;
    int32_t loadStartPosW;
    int32_t loadStartPosH;
    int32_t cropSizeW;
    int32_t cropSizeH;
    int8_t resizeSwitch;
    int32_t resizeOutputW;
    int32_t resizeOutputH;
    int8_t paddingSwitch;
    int32_t leftPaddingSize;
    int32_t rightPaddingSize;
    int32_t topPaddingSize;
    int32_t bottomPaddingSize;
    int8_t cscSwitch;
    int8_t rbuvSwapSwitch;
    int8_t axSwapSwitch;
    int8_t singleLineMode;
    int32_t matrixR0C0;
    int32_t matrixR0C1;
    int32_t matrixR0C2;
    int32_t matrixR1C0;
    int32_t matrixR1C1;
    int32_t matrixR1C2;
    int32_t matrixR2C0;
    int32_t matrixR2C1;
    int32_t matrixR2C2;
    int32_t outputBias0;
    int32_t outputBias1;
    int32_t outputBias2;
    int32_t inputBias0;
    int32_t inputBias1;
    int32_t inputBias2;
    int32_t meanChn0;
    int32_t meanChn1;
    int32_t meanChn2;
    int32_t meanChn3;
    float minChn0;
    float minChn1;
    float minChn2;
    float minChn3;
    float varReciChn0;
    float varReciChn1;
    float varReciChn2;
    float varReciChn3;
    aclFormat srcFormat;
    aclDataType srcDatatype;
    size_t srcDimNum;
    size_t shapeCount;
    aclAippDims outDims[ACL_MAX_SHAPE_COUNT];
    aclAippExtendInfo *aippExtend;
} aclAippInfo;
```

| Member Name | Description |
| --- | --- |
| inputFormat | Input image format. See [aclAippInputFormat](aclAippInputFormat.md) for the type definition. |
| srcImageSizeW | Width of the original image. |
| srcImageSizeH | Height of the original image. |
| cropSwitch | Crop switch. |
| loadStartPosW | Horizontal coordinate of the crop start point in the image. |
| loadStartPosH | Vertical coordinate of the crop start point in the image. |
| cropSizeW | Width of the crop area. |
| cropSizeH | Height of the crop area. |
| resizeSwitch | Resize switch. |
| resizeOutputW | Width of the image after resizing. |
| resizeOutputH | Height of the image after resizing. |
| paddingSwitch | Whether to pad the image. |
| leftPaddingSize | Padding value on the left side of the image. |
| rightPaddingSize | Padding value on the right side of the image. |
| topPaddingSize | Padding value on the top side of the image. |
| bottomPaddingSize | Padding value on the bottom side of the image. |
| cscSwitch | Color space conversion switch. |
| rbuvSwapSwitch | Switch that indicates whether to swap the R and B channels, or the U and V channels. |
| axSwapSwitch | Switch for RGBA-to-ARGB or YUVA-to-AYUV swapping. |
| singleLineMode | Single-line processing mode switch. Only the first line after cropping is processed. |
| matrixR0C0 | Color space conversion matrix parameter. |
| matrixR0C1 | Color space conversion matrix parameter. |
| matrixR0C2 | Color space conversion matrix parameter. |
| matrixR1C0 | Color space conversion matrix parameter. |
| matrixR1C1 | Color space conversion matrix parameter. |
| matrixR1C2 | Color space conversion matrix parameter. |
| matrixR2C0 | Color space conversion matrix parameter. |
| matrixR2C1 | Color space conversion matrix parameter. |
| matrixR2C2 | Color space conversion matrix parameter. |
| outputBias0 | Output bias for RGB-to-YUV conversion. |
| outputBias1 | Output bias for RGB-to-YUV conversion. |
| outputBias2 | Output bias for RGB-to-YUV conversion. |
| inputBias0 | Input bias for YUV-to-RGB conversion. |
| inputBias1 | Input bias for YUV-to-RGB conversion. |
| inputBias2 | Input bias for YUV-to-RGB conversion. |
| meanChn0 | Mean value of channel 0. |
| meanChn1 | Mean value of channel 1. |
| meanChn2 | Mean value of channel 2. |
| meanChn3 | Mean value of channel 3. |
| varReciChn0 | Reciprocal of the variance of channel 0. |
| varReciChn1 | Reciprocal of the variance of channel 1. |
| varReciChn2 | Reciprocal of the variance of channel 2. |
| varReciChn3 | Reciprocal of the variance of channel 3. |
| srcFormat | Input format of the original model before model conversion.<br>See aclFormat for the type definition. |
| srcDatatype | Input datatype of the original model before model conversion.<br>See aclDataType for the type definition. |
| srcDimNum | Input dims of the original model before model conversion. |
| shapeCount | Number of profiles in dynamic Shape (dynamic Batch or dynamic resolution) scenarios. |
| outDims | AIPP output dims information corresponding to each profile in a multi-shape model. This information is derived from the shape during model compilation and stored in the model. The output format, datatype, and output dims do not change with the shape.<br>See [aclAippDims](aclAippDims.md) for the type definition. |
| aippExtend | Reserved parameter. In the current version, you must pass a null pointer. |
