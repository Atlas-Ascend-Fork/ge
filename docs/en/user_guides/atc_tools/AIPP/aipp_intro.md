# What Is AIPP

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-16T09:06:36.953Z pushedAt=2026-10-29T08:20:30.981Z nl=2 -->

This section describes what AIPP is, its classification, and the features it provides.

<!-- @ref: ge/res/docs/zh/user_guides/atc_tools/AIPP/aipp_intro_res.md#id1 -->

AIPP (Artificial Intelligence Pre-Processing) is used to perform data preprocessing on the AI Core, including resizing images, color gamut conversion (converting the image format), and mean subtraction/coefficient multiplication (changing image pixels). After data preprocessing, actual model inference is performed.

This module provides functions similar to those of DVPP (Digital Vision Pre-Processing). Both can process media data, and their function scopes overlap. For example, both can resize images and convert image formats. However, their function scopes also differ. For example, DVPP can perform image encoding/decoding and video encoding/decoding, while AIPP can perform normalization configuration. Unlike DVPP, AIPP is mainly used to perform data preprocessing on the AI Core. DVPP is the image processing unit built into the AI processor and provides powerful hardware acceleration for media processing through media data processing APIs.

AIPP and DVPP can be used independently or in combination. In combined use, DVPP is generally used first to perform basic processing such as decoding, image matting, and resizing on images/videos. However, due to hardware constraints of DVPP, the image format and resolution after DVPP processing may not meet the model requirements. Therefore, AIPP is further used to perform color gamut conversion, image matting, padding, and other processing.

<!-- npu="310p" id2 -->
Atlas inference products scenario: The images output by the DVPP module are mostly aligned YUV420SP images, and DVPP does not support outputting RGB images. If the model requires RGB images, the service flow needs to use the AIPP module to convert the format of the aligned YUV420SP images and mat out the input images required by the model.
<!-- end id2 -->

AIPP is classified into static AIPP and dynamic AIPP based on the configuration method. To output the original image in an image format that meets the inference requirements, use the color gamut conversion function. To output an image of a fixed size, use the Crop (image matting) and Padding functions provided by AIPP.

## Static AIPP and Dynamic AIPP

When enabling AIPP, you can only configure either static AIPP or dynamic AIPP to process images, but not both at the same time. AIPP is enabled through the **aipp\_mode** parameter. For a specific configuration example, see [AIPP Configuration Example](aipp_config_example.md). For parameter description, see [Configuration File Template](config_file_template.md).

- Static AIPP: During model conversion, set the AIPP mode to static and configure the AIPP parameters. After the model is generated, the AIPP parameter values are saved in the offline model. Each model inference process uses the fixed AIPP preprocessing parameters, and these parameters cannot be directly modified through service code in subsequent inference processes.

    If static AIPP is used, the same AIPP parameters are shared across multiple batches.

- Dynamic AIPP: During model conversion, set the AIPP mode to dynamic. Before each inference, dynamically modify the AIPP parameter values as required, so that different AIPP parameters can be used during model execution. The dynamic AIPP parameter values are selected as appropriate for different service scenarios (for example, different cameras use different normalization parameters, and the input image format needs to be compatible with YUV420 and RGB).

    If dynamic AIPP is configured during model conversion, when using the application project for model inference, call the **aclmdlSetInputAIPP** API before the **aclmdlExecute** API provided by acl to set the dynamic AIPP data for model inference. For details about the API, see [Model Execution](../../../api/graph_engine_api/c/acl/model_execute_APIs.md).

    If dynamic AIPP is used, different parameters are used for multiple batches, which is reflected in the dynamic parameter structure, where each batch can be configured with different parameters such as crop. For details about the dynamic parameter structure, see [Input Structure of Dynamic AIPP Parameters](aipp_config_example.md#input-structure-of-dynamic-aipp-parameters).

The image input formats supported by AIPP include: YUV420SP\_U8, RGB888\_U8, XRGB8888\_U8, and YUV400\_U8.

## Color Gamut Conversion

Color gamut conversion is used to convert the input image format to the image format required by the model. When AIPP is enabled, the **csc\_switch** parameter controls whether color gamut conversion is enabled. For the parameter description, see [Configuration File Template](config_file_template.md).

Once the image formats before and after AIPP processing are determined, the values of other parameters related to color gamut conversion can be determined. This document provides relevant templates for your use without further modification. For a configuration example, see [Color Gamut Conversion Configuration](CSC_config.md).

## Changing the Image Size

The image size change operation in AIPP is implemented by Crop (image matting) and Padding, which correspond to the crop and padding parameters in the configuration template. For parameter description, see [Configuration File Template](config_file_template.md).

For details about this function and AIPP parameter configuration examples, see [Crop/Padding Configuration](crop_padding_config.md).

## Validation of Model Input Size by AIPP

If AIPP is configured, regardless of whether it is static AIPP or dynamic AIPP, the input size of the final generated offline model (that is, input\_size) is affected by operations such as Crop and Padding. This section describes the constraints on the model input size.

Assume that the batch size of the model is N (in a dynamic batch scenario, N is the value of the maximum profile), the width of the model input image is src\_image\_size\_w, and the height is src\_image\_size\_h. The formula for calculating the final model input size is as follows.

### Validation of Model Input Size by Static AIPP

- **YUV400\_U8**

    ```text
    N * src_image_size_w * src_image_size_h * 1
    ```

- **YUV420SP\_U8**

    ```text
    N * src_image_size_w * src_image_size_h * 1.5
    ```

- **XRGB8888\_U8**

    ```text
    N * src_image_size_w * src_image_size_h * 4
    ```

- **RGB888\_U8**

    ```text
    N * src_image_size_w * src_image_size_h * 3
    ```

### Validation of Model Input Size by Dynamic AIPP

For dynamic AIPP, during model conversion, ATC adds a model input for dynamic AIPP to receive the AIPP parameters passed in after the **aclmdlSetInputAIPP** API is called during model inference. In this scenario, the size of the added input node is calculated using the following formula. For details about the API, see [aclmdlSetInputAIPP](../../../api/graph_engine_api/c/acl/aclmdlSetInputAIPP.md).

```text
sizeof(kAippDynamicPara) - sizeof(kAippDynamicBatchPara) + batch_count * sizeof(kAippDynamicBatchPara)
```

For the parameter description of kAippDynamicPara and kAippDynamicBatchPara, see [Input Structure of Dynamic AIPP Parameters](aipp_config_example.md#input-structure-of-dynamic-aipp-parameters).
