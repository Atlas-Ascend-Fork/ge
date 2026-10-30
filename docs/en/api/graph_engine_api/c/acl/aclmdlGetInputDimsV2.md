# aclmdlGetInputDimsV2

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:16:00.801Z pushedAt=2026-10-29T03:17:45.348Z nl=2 -->

## Product Support

<!-- npu="950" id1168 -->
- Ascend 950PR&950DT products: Supported
<!-- end id1168 -->
<!-- npu="A3" id1169 -->
- Atlas A3 products: Supported
<!-- end id1169 -->
<!-- npu="910b" id1170 -->
- Atlas A2 products: Supported
<!-- end id1170 -->
<!-- npu="310b" id1171 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id1171 -->
<!-- npu="310p" id1172 -->
- Atlas inference products: Supported
<!-- end id1172 -->
<!-- npu="910" id1173 -->
- Atlas training products: Supported
<!-- end id1173 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlGetInputDimsV2_res.md#id1 -->

## Feature Description

Obtains the dimension information of the model input tensor based on the model description.

If the model contains static AIPP configuration information, you can choose either [aclmdlGetInputDims](aclmdlGetInputDims.md) or [aclmdlGetInputDimsV2](aclmdlGetInputDimsV2.md) to query the dimension information as needed. The differences between the two APIs are as follows:

- The dimension information obtained through [aclmdlGetInputDims](aclmdlGetInputDims.md) keeps the value of each dimension consistent with that of the corresponding dimension of the input image. For details, see [Table 1](aclmdlGetInputDims.md).
- For the dimension information obtained through [aclmdlGetInputDimsV2](aclmdlGetInputDimsV2.md), the value of the H dimension = the H dimension value of the input image \* coefficient. The value of the C dimension varies with the image format. For details, see [Table 1](aclmdlGetInputDimsV2.md) in the [aclmdlGetInputDimsV2](aclmdlGetInputDimsV2.md) API. The product of all dimension values is consistent with the value obtained through [aclmdlGetInputSizeByIndex](aclmdlGetInputSizeByIndex.md).

## Prototype

```c
aclError aclmdlGetInputDimsV2(const aclmdlDesc *modelDesc, size_t index, aclmdlIODims *dims)
```

## Parameters

| Parameter Name | Input/Output | Description |
| --- | --- | --- |
| modelDesc | Input | Pointer to the aclmdlDesc type.<br>Call [aclmdlCreateDesc](aclmdlCreateDesc.md) in advance to create data of the aclmdlDesc type. |
| index | Input | Specifies the index of the input whose dims are to be obtained. The index value starts from 0. |
| dims | Output | Pointer to the input dimension information. For the type definition, see [aclmdlIODims](aclmdlIODims.md).<br>For dynamic batch and dynamic resolution (width and height) scenarios, the batch size or width and height in the dims of the input tensor is -1, indicating that it is dynamically variable. For example, if the format of the input tensor is NCHW, in a dynamic batch scenario, the dims of the dynamically variable input tensor is [-1,3,224,224]; in a dynamic resolution scenario, the dims of the dynamically variable input tensor is [1,3,-1,-1]. The italic parts in the examples are subject to the actual situation.<br>If the length of the tensor name is greater than 127, when outputting dims.name, the API converts the tensor name to the "acl_modelId_${id}_input_${index}_${random string}  " format (if the converted tensor name conflicts with an existing tensor name in the model, "_${random string}  " is appended to the end of the converted name; otherwise, no random string is added), and establishes a mapping between the converted name and the original name. You can call [aclmdlGetTensorRealName](aclmdlGetTensorRealName.md) and pass the converted name to obtain the original name (if you pass the original name to the API, the original name is still returned). If the length of the tensor name is less than or equal to 127, the tensor name is output as dims.name.<br>For static AIPP scenarios, this API defines the dimension definition rules for each dimension based on the image format and the corresponding NHWC format, as shown in the following table. |

**Table 1**  Dimension definition rules in static AIPP scenarios

| Image Format | Format Reference Format | Dimension Definition Rules |
| --- | --- | --- |
| YUV420SP_U8 | NHWC | n,h*1.5,w,1 |
| XRGB8888_U8 | NHWC | n,h,w,4 |
| RGB888_U8 | NHWC | n,h,w,3 |
| YUV400_U8 | NHWC | n,h,w,1 |
| ARGB8888_U8 | NHWC | n,h,w,4 |
| YUYV_U8 | NHWC | n,h,w,2 |
| YUV422SP_U8 | NHWC | n,h*2,w,1 |
| AYUV444_U8 | NHWC | n,h,w,4 |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
