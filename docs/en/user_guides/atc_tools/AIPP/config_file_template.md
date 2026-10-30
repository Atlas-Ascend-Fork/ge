# Configuration File Template

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-16T09:07:08.340Z pushedAt=2026-10-20T03:59:13.480Z -->

The AIPP configuration file is configured using the template provided in this section. Its content must comply with the prototxt format. You can determine which parameters to configure based on your scenario, modify them to appropriate values, save the file, and then use it for model conversion. Before using the configuration template, check the related constraints.

## Overall Constraints on Template Usage

- **When using the configuration file template, uncomment the parameters to be configured and set them to appropriate values.**
- **The parameter values in the template are all default values. In actual use, if some parameters in the configuration file are not configured, they are automatically set to the default values of the corresponding parameters in the template during model conversion.**
- **In static AIPP scenarios, the input\_format attribute is mandatory, and all other attributes are optional. If they are not configured, they are automatically set to the default values of the corresponding parameters in the template during model conversion.**
- **Due to hardware processing logic limitations, the parameters in the configuration file must be processed in the following order: channel swap (rbuv\_swap\_switch) \> image cropping (crop) \> color space conversion (channel swap) \> mean subtraction/normalization \> image padding.**
- AIPP currently supports color space conversion, image cropping, mean subtraction, coefficient multiplication, channel swap, and single-line mode. **The input image type supports only RAW and UINT8 formats.**

<!-- npu="950,A3,910b,910,310p,310b" id3 -->
- The cropping constraints are as follows:
  - If input_format is set to YUV420SP_U8, load_start_pos_w and load_start_pos_h must be even numbers. The Ascend 950PR&950DT products have no even-number requirement. If input_format is set to other values, there is no constraint on load_start_pos_w and load_start_pos_h.
  - If the cropping function is enabled, src_image_size[W|H] >= crop_size[W|H]+load_start_pos[W|H].
<!-- end id3 -->

<!-- @ref: ge/res/docs/zh/user_guides/atc_tools/AIPP/config_file_template_res.md#id3 -->

- Padding constraints:
  - After AIPP padding, the output H and W must be consistent with the H and W required by the model.
  <!-- @ref: ge/res/docs/zh/user_guides/atc_tools/AIPP/config_file_template_res.md#id4 -->
  <!-- npu="910,310p" id4 -->
  - For Atlas Inference Series and Atlas Training Series, the value of W must be <= 1080.
  <!-- end id4 -->
  <!-- npu="950,A3,910b,310b" id5 -->
  - For Atlas 200I/500 A2 inference product, Atlas A2 products, Atlas A3 products, and Ascend 950PR&950DT products, the value of W must be <= 4096.
  <!-- end id5 -->
<!-- npu="950,A3,910b,910,310p,310b" id1 -->
- This version does not support the raw_rgbir_to_f16_n configuration item.
<!-- end id1 -->
- If the input image is RGB (an image composed of the R, G, and B components), the corresponding input and output channel order, from high address to low address, is as follows: \{R,G,B\}.

- For dynamic AIPP, the parameters need to be calculated for each inference, which takes time. Therefore, the performance of dynamic AIPP is lower than that of static AIPP.

- Images processed by AIPP are uniformly stored in the NC1HWC0 five-dimensional data format:

  The following uses an image required by the original model as RGB (an image composed of the R, G, and B components) as an example to describe the scenario where the AIPP function is configured:

  - For the Caffe/ONNX framework, the data format can only be set to NCHW (data storage format: RRRRRRGGGGGGBBBBBB).
  - For the TensorFlow framework, the data format can only be set to NHWC (data storage format: RGBRGBRGBRGBRGBRGB) or NCHW (data storage format: RRRRRRGGGGGGBBBBBB).

    After the actually provided image is processed by the AIPP color space conversion function, the image in the output offline model is RGB and is stored in the NC1HWC0 five-dimensional data format (for details about NC1HWC0, see [Key Concepts](../overview/atc_overview.md#key-concepts)): if the AIPP output data type is FP16, C0=16, and the bits from high to low are R, G, and B, with the remaining bits padded with 0; C1=1.

- Whether AIPP is enabled during model conversion determines the requirements on the input image data during inference:
  - AIPP enabled during model conversion: during inference, the input image data must be in NHWC layout. In this scenario, the format of the input node that is ultimately connected to AIPP is forcibly changed to NHWC, which may be inconsistent with the format specified by the [--input\_format](../CLI_options/--input_format.md) parameter in the model conversion command.
  - AIPP not enabled during model conversion: during inference, the Format of the model must be consistent with the Format of the input image. For example, if the Format of the input image is NHWC but the default Format of a model is NCHW, the Format of the input image is inconsistent with that of the model. In this case, you can specify [--input\_format](../CLI_options/--input_format.md) during model conversion to adjust the Format of the model, or select an input image that meets the model requirements.

- For the input image format YUV420SP, depending on the order of the UV components, YUV420SP is further divided into YUV420SP\_UV\(NV12\) and YUV420SP\_VU\(NV21\), which correspond to YUV420SP\_U8 and YVU420SP\_U8 in [Color Space Conversion Configuration](CSC_config.md), respectively. The default is YUV420SP\_UV\(NV12\).

  For the input\_format parameter in the AIPP configuration file, it must always be configured as the NV12 format (YUV420SP\_U8). Use the rbuv\_swap\_switch parameter to control the image format actually provided to AIPP:

  - If rbuv\_swap\_switch is set to false, the actually provided image format is YUV420SP\_U8.
  - If rbuv\_swap\_switch is set to true, the actually provided image format is YVU420SP\_U8.

- Constraints on the C-axis value corresponding to different AIPP image formats.

    **Table 1**  C-axis values corresponding to different image formats

    | AIPP input image format (input_format) | C-axis value |
    | --- | --- |
    | YUV420SP_U8 | C=1 |
    | RGB888_U8 | C=3 |
    | XRGB8888_U8 | C=4 |
    | YUV400_U8 | C=1 |

- The typical parameter configuration of AIPP for various image formats is shown in the following table.

    **Table 2** Typical parameter configuration for various image formats

    | AIPP input image format (input_format) | Input image memory layout format | Corresponding original input image format | AIPP output format | Description of related parameters in the AIPP configuration file |
    | --- | --- | --- | --- | --- |
    | RGB888_U8 | NHWC | RGB package | NC1HWC0 | rbuv_swap_switch is generally set to false |
    | RGB888_U8 | NHWC | BGR package | NC1HWC0 | rbuv_swap_switch is generally set to true, and internal conversion to RGB package is performed first before subsequent processing |
    | YUV420SP_U8 | / | YUV420 sp NV12 8bit | NC1HWC0 | rbuv_swap_switch is generally set to false |
    | YUV420SP_U8 | / | YUV420 sp NV21 8bit | NC1HWC0 | rbuv_swap_switch is generally set to true, and internal conversion to NV12 format is performed first before subsequent processing |
    | YUV400_U8 | NHWC | Grayscale image | NC1HWC0 | / |
    | XRGB8888_U8 | NHWC | - XRGB package<br>  - XBGR package<br>  - RGBX package<br>  - BGRX package | NC1HWC0 | - XRGB package: rbuv_swap_switch is generally set to false, and ax_swap_switch is generally set to true<br>  - XBGR package: rbuv_swap_switch is generally set to true, ax_swap_switch is generally set to true, and internal conversion to XRGB package is performed first before subsequent processing<br>  - RGBX package: rbuv_swap_switch is generally set to false, ax_swap_switch is generally set to false, and internal conversion to XRGB package is performed first before subsequent processing<br>  - BGRX package: rbuv_swap_switch is generally set to true, ax_swap_switch is generally set to false, and internal conversion to XRGB package is performed first before subsequent processing |

## Configuration File Template

```textproto
# The AIPP configuration starts with aipp_op, which identifies the configuration of an AIPP operator. Multiple aipp_op configurations are supported.
aipp_op {

#========================= Global settings (start) ===========================================================================================================================================================
# aipp_mode specifies the AIPP mode. This parameter is mandatory.
# Type: enum
# Value range: dynamic/static. dynamic indicates dynamic AIPP, and static indicates static AIPP.
aipp_mode:

# related_input_rank is optional. It identifies which input of the model AIPP processing is performed on, starting from 0, and the default value is 0. For example, if a model has two inputs and AIPP needs to be performed on the second input, set related_input_rank to 1.
# Type: integer
# Value range >= 0
related_input_rank: 0

# related_input_name is optional. It identifies which input of the model AIPP processing is performed on. Set this parameter to the name of the model input (the value corresponding to input) or the output of the first-layer node of the model (the value corresponding to the top parameter). This parameter applies only to Caffe network models and cannot be used together with related_input_rank. If both are configured, related_input_name takes precedence over related_input_rank.
# For example, if a model has two inputs named data and im_info, and AIPP needs to be performed on the second input, set related_input_name to im_info.
# Type: string
# Value range: none
related_input_name: ""

#========================= Global settings (end) =============================================================================================================================================================

#========================= Required for dynamic AIPP, not required for static AIPP (start) ===================================================================================================================================
# Maximum size of the input image. This parameter must be configured for dynamic AIPP. (In a dynamic batch scenario, N is the value of the maximum number of gears.)
# Type: int
max_src_image_size: 0
# If the input image format is YUV400_U8, max_src_image_size >= N * src_image_size_w * src_image_size_h * 1.
# If the input image format is YUV420SP_U8, max_src_image_size >= N * src_image_size_w * src_image_size_h * 1.5.
# If the input image format is XRGB8888_U8, max_src_image_size >= N * src_image_size_w * src_image_size_h * 4.
# If the input image format is RGB888_U8, max_src_image_size >= N * src_image_size_w * src_image_size_h * 3.

# Whether rotation is supported. This is a reserved field and the function is not supported yet.
# Type: bool
# Value range: true/false. true indicates that rotation is supported, and false indicates that rotation is not supported.
support_rotation: false
#========================= Set for dynamic AIPP; not required for static AIPP (end) =======================================================================================================================================

#========================= Set for static AIPP; not required for dynamic AIPP (start)======================================================================================================================================
# Input image format. Mandatory.
# Type: enum
# Value range: YUV420SP_U8, XRGB8888_U8, RGB888_U8, YUV400_U8

input_format:
# Note: After model conversion is complete, the preceding parameters are presented as enumeration values 1, 2, 3, and 4 respectively in the corresponding offline OM model file.

# Width and height of the original image
# Type: int32
# Value range and constraints: The width ranges from [2, 4096] or is 0. The height ranges from [1, 4096] or is 0. For images of the YUV420SP_U8 type, the width and height of the original image must be even numbers.
src_image_size_w: 0
src_image_size_h: 0
# Description: Configure src_image_size_w and src_image_size_h based on the actual width and height of the image. Only when neither crop nor padding is enabled can src_image_size_w and src_image_size_h be set to 0 or left unconfigured. In this case, the w and h defined by the network model input are used, where w ranges from [2, 4096] and h ranges from [1, 4096].

# Padding value in the C dimension. Reserved field. This function is not supported yet.
# Type: float16
# Value range: [-65504, 65504]
cpadding_value: 0.0

#========= Crop parameter settings (for configuration examples, see AIPP Configuration > Crop/Padding Configuration) =========
# Whether cropping is supported when AIPP processes images
# Type: bool
# Value range: true/false. true indicates supported, and false indicates not supported.
crop: false

# Horizontal and vertical coordinates of the crop start position. The crop size is the w and h defined by the network input.
# Type: int32
# Value range and constraints: [0,4095]
# Description: load_start_pos_w<src_image_size_w, load_start_pos_h<src_image_size_h
load_start_pos_w: 0
load_start_pos_h: 0

# Image size after cropping
# Type: int32
# Value range and constraints: [0,4096], load_start_pos_w + crop_size_w <= src_image_size_w, load_start_pos_h + crop_size_h <= src_image_size_h
crop_size_w: 0
crop_size_h: 0
Description: If cropping is enabled and padding is not configured, crop_size_w and crop_size_h can be set to 0 or left unconfigured only in this scenario. In this case, the width and height of the crop size (crop_size[W|H]) are obtained from the width and height in --input_shape of the model file, and the value range of the width and height in --input_shape is [1,4096].

#================================== resize parameter settings ================================
# Whether AIPP supports scaling when processing images. Reserved field. This function is not supported yet.
# Type: bool
# Value range: true/false. true indicates supported, false indicates not supported.
resize: false

# Width and height of the image after scaling. Reserved field. This function is not supported yet.
# Type: int32
# Value range & constraints: resize_output_h: [16,4096] or 0; resize_output_w: [16,1920] or 0; resize_output_w/resize_input_w∈[1/16,16], resize_output_h/resize_input_h∈[1/16,16]
resize_output_w: 0
resize_output_h: 0
# Note: If scaling is enabled and padding is not configured, resize_output_w and resize_output_h can be set to 0 or left unconfigured only in this scenario. In this case, the width and height of the scaled image are taken from the width and height in --input_shape of the model file, where the height in --input_shape ranges from [16,4096] and the width ranges from [16,1920].


#======== padding parameter settings (for configuration examples, see AIPP Configuration > Crop/Padding Configuration) =========
# Switch for enabling padding when AIPP processes images
# Type: bool
# Value range: true/false. true indicates supported, and false indicates not supported.
padding: false

# Padding values of H and W, static AIPP configuration
# Type: int32
# Value range: [0,32]
left_padding_size: 0
right_padding_size: 0
top_padding_size: 0
bottom_padding_size: 0
# Description: After AIPP padding, the output H and W must be consistent with the H and W required by the model.


# Pixel values for padding in the top, bottom, left, and right directions, static AIPP configuration
# Type: uint8/int8/float16
# Value ranges: [0,255], [-128, 127], and [-65504, 65504], respectively.
padding_value: 0
# Description: The value of this parameter must be consistent with the data type of the final AIPP output image.


#================================ rotation parameter settings ==================================
# Rotation angle for AIPP image processing. Reserved field. This function is not supported yet.
# Type: uint8
# Range: {0, 1, 2, 3}. 0: no rotation, 1: 90° clockwise, 2: 180° clockwise, 3: 270° clockwise.
rotation_angle: 0


#========= Color space conversion parameter settings (for a configuration example, see AIPP Configuration > Color Space Conversion Configuration) =============
# Color space conversion switch, static AIPP configuration
# Type: bool
# Value range: true/false. true: enable color space conversion, false: disable it.
csc_switch: false

# R channel and B channel swap switch / U channel and V channel swap switch
# Type: bool
# Value range: true/false. true: enable channel swap, false: disable it.
rbuv_swap_switch: false

# RGBA->ARGB, YUVA->AYUV swap switch
# Type: bool
# Value range: true/false. true indicates enabled, and false indicates disabled.
ax_swap_switch: false

# Switch for single-line processing mode (only the first line after cropping is processed). Reserved field. This function is not supported yet.
# Type: bool
# Value range: true/false. true indicates that single-line processing mode is enabled, and false indicates that it is disabled.
single_line_mode: false

# If the color space conversion switch is set to false, this function does not take effect.
# If the number of channels of the input image is 4, the A channel or X channel is ignored.
# YUV to BGR:
# | B |   | matrix_r0c0 matrix_r0c1 matrix_r0c2 | | Y - input_bias_0 |
# | G | = | matrix_r1c0 matrix_r1c1 matrix_r1c2 | | U - input_bias_1 | >> 8
# | R |   | matrix_r2c0 matrix_r2c1 matrix_r2c2 | | V - input_bias_2 |
# BGR to YUV:
# | Y |   | matrix_r0c0 matrix_r0c1 matrix_r0c2 | | B |        | output_bias_0 |
# | U | = | matrix_r1c0 matrix_r1c1 matrix_r1c2 | | G | >> 8 + | output_bias_1 |
# | V |   | matrix_r2c0 matrix_r2c1 matrix_r2c2 | | R |        | output_bias_2 |

# 3*3 CSC matrix elements
# Type: int16
# Value range: [-32677, 32676]
matrix_r0c0: 298
matrix_r0c1: 516
matrix_r0c2: 0
matrix_r1c0: 298
matrix_r1c1: -100
matrix_r1c2: -208
matrix_r2c0: 298
matrix_r2c1: 0
matrix_r2c2: 409

# Output offset for RGB-to-YUV conversion
# Type: uint8
# Value range: [0, 255]
output_bias_0: 16
output_bias_1: 128
output_bias_2: 128

# Input offset for YUV-to-RGB conversion
# Type: uint8
# Value range: [0, 255]
input_bias_0: 16
input_bias_1: 128
input_bias_2: 128


#============================== Mean Subtraction and Coefficient Multiplication Settings =================================
# The calculation rules are as follows:
# This function does not take effect for uint8-to-uint8 conversion.
# For uint8-to-fp16 conversion, pixel_out_chx(i) = [pixel_in_chx(i) – mean_chn_i – min_chn_i] * var_reci_chn_i

# Mean value of each channel
# Type: uint8
# Value range: [0, 255]
mean_chn_0: 0
mean_chn_1: 0
mean_chn_2: 0
mean_chn_3: 0

# Minimum value of each channel
# Type: float16
# Value range: [0, 255]
min_chn_0: 0.0
min_chn_1: 0.0
min_chn_2: 0.0
min_chn_3: 0.0

# Reciprocal of the variance of each channel
# Type: float16
# Value range: [-65504, 65504]
var_reci_chn_0: 1.0
var_reci_chn_1: 1.0
var_reci_chn_2: 1.0
var_reci_chn_3: 1.0

# RGB16/RGB20/RGB24/RGB8_IR/RGB16_IR/RGB20_IR->fp16, parameters for converting the RGBIR format to fp16
# Type: uint32
# Value range: [0,31], default is 8
raw_rgbir_to_f16_n: 8
# Formula for converting the raw format to fp16: pixel_out_chx(i) = Max((fp16(rounding(FP32(UINT24(pixel_in_chx(i)) - mean_chn_i)/(2^raw_rgbir_to_f16_n))) - min_chn_i), 0)* var_reci_chn_i


#========================= Required for static AIPP, not required for dynamic AIPP (end)=====================================================================================================================================

}
```
