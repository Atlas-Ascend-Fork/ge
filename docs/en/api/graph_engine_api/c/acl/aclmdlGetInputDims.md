# aclmdlGetInputDims

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:15:23.934Z pushedAt=2026-10-29T03:14:33.602Z nl=2 -->

## Product Support

<!-- npu="950" id615 -->
- Ascend 950PR&950DT products: Supported
<!-- end id615 -->
<!-- npu="A3" id616 -->
- Atlas A3 products: Supported
<!-- end id616 -->
<!-- npu="910b" id617 -->
- Atlas A2 products: Supported
<!-- end id617 -->
<!-- npu="310b" id618 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id618 -->
<!-- npu="310p" id619 -->
- Atlas inference products: Supported
<!-- end id619 -->
<!-- npu="910" id620 -->
- Atlas training products: Supported
<!-- end id620 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlGetInputDims_res.md#id1 -->

## Feature Description

Obtains the dimension information of the input tensor of a model based on the model description.

If the model contains static AIPP configuration information, you can use either [aclmdlGetInputDims](aclmdlGetInputDims.md) or [aclmdlGetInputDimsV2](aclmdlGetInputDimsV2.md) to query the dimension information as needed. The differences between the two APIs are as follows:

- The dimension values obtained through [aclmdlGetInputDims](aclmdlGetInputDims.md) are consistent with the dimension values of the input image.
- For the dimension information obtained through [aclmdlGetInputDimsV2](aclmdlGetInputDimsV2.md), the H dimension value = the H dimension value of the input image \* coefficient. The C dimension value varies with the image format, and the product of all dimension values is consistent with the value obtained through [aclmdlGetInputSizeByIndex](aclmdlGetInputSizeByIndex.md).

## Prototype

```c
aclError aclmdlGetInputDims(const aclmdlDesc *modelDesc, size_t index, aclmdlIODims *dims)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| modelDesc | Input | Pointer to the aclmdlDesc type.<br>Call the [aclmdlCreateDesc](aclmdlCreateDesc.md) API in advance to create data of the aclmdlDesc type. |
| index | Input | Specifies the index of the input whose dims are to be obtained. The value of index starts from 0. |
| dims | Output | Pointer to the input dimension information. See [aclmdlIODims](aclmdlIODims.md) for the type definition.<br>In dynamic batch and dynamic resolution (width and height) scenarios, the batch size or width and height in the dims of the input tensor is -1, indicating that it is dynamically variable. For example, if the format of the input tensor is NCHW, in a dynamic batch scenario, the dims of the dynamically variable input tensor is [-1,3,224,224]; in a dynamic resolution scenario, the dims of the dynamically variable input tensor is [1,3,-1,-1]. The italicized parts in the examples are subject to the actual situation.<br>If the length of the tensor name is greater than 127, when outputting dims.name, the API converts the tensor name to the "acl_modelId_${id}_input_${index}_${random string}  " format (if the converted tensor name conflicts with an existing tensor name in the model, "_${random string}  " is appended to the end of the converted name; otherwise, no random string is appended), and establishes a mapping between the converted name and the original name. You can call the [aclmdlGetTensorRealName](aclmdlGetTensorRealName.md) API and pass the converted name to obtain the original name (if you pass the original name to the API, the original name is still returned). If the length of the tensor name is less than or equal to 127, the tensor name is output as is when outputting dims.name.<br>In static AIPP scenarios, for images in different formats, this API corresponds to the NHWC format and defines the dimension definition rule for each dimension, as shown in the following table. |

**Table 1**  Dimension definition rule in static AIPP scenarios

| Image Format | Format Reference | Dimension Definition Rule |
| --- | --- | --- |
| YUV420SP_U8 | NHWC | n,h,w,c |
| XRGB8888_U8 | NHWC | n,h,w,c |
| RGB888_U8 | NHWC | n,h,w,c |
| YUV400_U8 | NHWC | n,h,w,c |
| ARGB8888_U8 | NHWC | n,h,w,c |
| YUYV_U8 | NHWC | n,h,w,c |
| YUV422SP_U8 | NHWC | n,h,w,c |
| AYUV444_U8 | NHWC | n,h,w,c |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
