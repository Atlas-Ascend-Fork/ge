# How to Enable AIPP

<!-- md-trans-meta sourceCommit=3b4c8f106b74c3a75512dd245c2b55b143c90db0 translatedAt=2026-10-16T09:11:03.988Z pushedAt=2026-10-22T09:28:07.495Z -->

By enabling the AIPP function during model conversion, all data processing can be completed before inference. Because a dedicated acceleration module is used to implement and guarantee performance, image processing does not become a bottleneck in the inference phase, and the image processing method is flexible. This section describes how to enable the AIPP function during model conversion.

This section uses the ResNet50 network model of the TensorFlow framework as an example to demonstrate how to enable the static AIPP function through model conversion. After the AIPP function is enabled, if the test image actually provided for model inference does not meet the requirements (including image format and image size), an image that meets the model requirements is output after model conversion, and this information is fixed into the converted offline model (after model conversion, the AIPP function is inserted into the offline model in the form of an Aipp operator).

The ResNet50 network model requires the image format to be RGB and the image size to be 224\*224. In addition, assume that the test image provided for model inference has a size of 250\*250 and a format of YUV420SP, and the valid data area starts from the pixel at the upper-left corner \(0, 0\). The operations required to enable AIPP are analyzed in [Table 1](#table1).

**Table 1** Scenario analysis<a id="table1"></a>

| Category | ResNet50 network model requirement | Test image actually provided for model inference | Required operation |
| --- | --- | --- | --- |
| Image format | RGB | YUV420SP | In this scenario, enable the color space conversion function of AIPP to convert the YUV420SP format to the RGB format required by the model. For details about the color space conversion function, see [Color Space Conversion Configuration](CSC_config.md). |
| Image size | 224*224 | 250*250 | The provided test image size 250*250 is larger than 224*224. In this scenario, enable the AIPP crop function, and set the horizontal and vertical coordinates load_start_pos_h and load_start_pos_w of the crop start position to 0. During inference, data in the 224*224 area is selected starting from point (0, 0). |

The detailed implementation procedure is as follows:

1. Obtain the TensorFlow network model.

    Click [Link](https://gitee.com/ascend/ModelZoo-TensorFlow/tree/master/TensorFlow/contrib/cv/resnet50_for_TensorFlow) and follow the on-page instructions to obtain the model file (\*.pb) of the ResNet50 network. Then, as a user running the CANN software package, upload the obtained file to any directory in the development environment, for example, the $HOME/module/ directory.

2. Construct the AIPP configuration file insert\_op.cfg.

    A static AIPP configuration template mainly consists of the following parts: the AIPP configuration mode (static AIPP or dynamic AIPP), the original image information (including the image format and image size), image resizing (crop and padding), and color space conversion. The following describes how to configure each part.

    1. The AIPP configuration mode is determined by the aipp\_mode parameter. The following is an example configuration in a static scenario:

        ```textproto
               aipp_mode : static           #static indicates that the AIPP is configured as static
        ```

    2. Configure the original image information.

        ```textproto
               input_format : YUV420SP_U8     #Format of the original image input to AIPP
               src_image_size_w : 250         #Width and height of the original image input to AIPP
               src_image_size_h : 250
        ```

    3. Resize the image.

        Image resizing is implemented by functions such as crop and padding. In this example, you need to configure the crop start position and the size of the cropped image. If the cropped image size still does not meet the model requirements, you also need to configure padding.

        AIPP provides a more convenient configuration method. If crop is enabled and padding is not configured, the crop size can be set to 0 or left unconfigured. In this case, the width and height of the crop size are obtained from the width and height in the model --input\_shape. In this example, the crop size is not configured. The following is an example configuration:

        ```textproto
               crop: true                     #Crop switch, used to resize the image
               load_start_pos_h: 0            #Horizontal and vertical coordinates of the crop start position
               load_start_pos_w: 0
        ```

    4. Color space conversion.

        The color space conversion function is controlled by the csc\_switch parameter and used together with parameters such as the color space conversion coefficient matrix\_r\*c\* and channel swap rbuv\_swap\_switch. AIPP provides a convenient feature: once the image formats before and after AIPP processing are determined, the parameter values related to color space conversion can be determined, and **users do not need to modify them**. That is, all the preceding parameters can be directly copied from the template. For the template example and more configuration templates, see [Color Space Conversion Configuration](CSC_config.md). The following is a configuration example for this scenario:

        ```textproto
               csc_switch : true              #Color space conversion switch. true indicates that color space conversion is enabled.
               rbuv_swap_switch : false       #Channel swap switch (R/B channel swap switch and U/V channel swap switch). In this example, no channel swap is involved, so this parameter is set to false. The default value is false.
               matrix_r0c0 : 256              #Color space conversion coefficient
               matrix_r0c1 : 0
               matrix_r0c2 : 359
               matrix_r1c0 : 256
               matrix_r1c1 : -88
               matrix_r1c2 : -183
               matrix_r2c0 : 256
               matrix_r2c1 : 454
               matrix_r2c2 : 0
               input_bias_0 : 0
               input_bias_1 : 128
               input_bias_2 : 128
        ```

    Combine all the preceding parameters into the insert\_op.cfg file to construct the AIPP configuration file. The complete example is as follows:

    ```textproto
    aipp_op {
           aipp_mode : static             #AIPP configuration mode
           input_format : YUV420SP_U8     #Format of the original image input to AIPP
           src_image_size_w : 250         #Width and height of the original image input to AIPP
           src_image_size_h : 250
           crop: true                     #Crop switch, used to change the image size
           load_start_pos_h: 0            #Horizontal and vertical coordinates of the crop start position
           load_start_pos_w: 0
           csc_switch : true              #Color space conversion switch. true indicates that color space conversion is enabled.
           rbuv_swap_switch : false       #Channel swap switch
           matrix_r0c0 : 256              #Color space conversion coefficient. No modification is required.
           matrix_r0c1 : 0
           matrix_r0c2 : 359
           matrix_r1c0 : 256
           matrix_r1c1 : -88
           matrix_r1c2 : -183
           matrix_r2c0 : 256
           matrix_r2c1 : 454
           matrix_r2c2 : 0
           input_bias_0 : 0
           input_bias_1 : 128
           input_bias_2 : 128
    }
    ```

    You can obtain more AIPP configuration examples for various scenarios from [AIPP Configuration Examples](aipp_config_example.md) or [Typical Scenario Examples](typical_scenario_examples.md). If the preceding examples still cannot meet your requirements, see [Configuration File Template](config_file_template.md) to construct a configuration file by yourself. Upload the insert\_op.cfg file to the Linux server where the ATC tool is located.

3. Add the [--insert\_op\_conf](../CLI_options/--insert_op_conf.md) parameter to the atc command to insert the AIPP preprocessing operator. Run the following command to generate an offline model. (The directories and files used in the following command are examples. Use the actual ones.)

    ```bash
    atc --model=$HOME/module/resnet50_tensorflow.pb --framework=3 --output=$HOME/module/out/tf_resnet50 --soc_version=<soc_version> --insert_op_conf=$HOME/module/insert_op.cfg
    ```

    For details about the parameters and their usage, see [Parameter Description](../CLI_options/README.md). If the following information is displayed, the model conversion is successful.

    ```console
    ATC run success, welcome to the next use.
    ```

    After the command is successfully executed, you can view the offline model (for example, *tf\_resnet50*.om) in the path specified by the --output parameter.

4. (Optional) To view the information about the Aipp operator in the converted offline model, convert the offline model to a JSON file. The command is as follows:

    ```bash
    atc --mode=1 --om=$HOME/module/out/tf_resnet50.om  --json=$HOME/module/out/tf_resnet50.json
    ```

    The following is a sample of the aipp information in the JSON file (all aipp attribute values in the following sample are examples only; use the configuration file actually constructed by the user as the standard):

    ```json
    {
                  "key": "aipp",
                  "value": {
                    "func": {
                      "attr": [
                        {
                          "key": "mean_chn_0",
                          "value": {
                            "i": 0
                          }
                        },
                        {
                          "key": "mean_chn_1",
                          "value": {
                            "i": 0
                          }
                        },
                        {
                          "key": "mean_chn_2",
                          "value": {
                            "i": 0
                          }
                        },
                        {
                          "key": "mean_chn_3",
                          "value": {
                            "i": 0
                          }
                        },
                        {
                          "key": "csc_switch",
                          "value": {
                            "b": true
                          }
                        },
                        {
                          "key": "input_format",
                          "value": {
                            "i": 1
                          }
                        },
                        {
                          "key": "input_bias_0",
                          "value": {
                            "i": 0
                          }
                        },
                        {
                          "key": "input_bias_1",
                          "value": {
                            "i": 128
                          }
                        },
                        {
                          "key": "input_bias_2",
                          "value": {
                            "i": 128
                          }
                        },
                        {
                          "key": "aipp_mode",
                          "value": {
                            "i": 1
                          }
                        },
                        {
                          "key": "src_image_size_h",
                          "value": {
                            "i": 250
                          }
                        },
                        {
                          "key": "crop_size_h",
                          "value": {
                            "i": 0
                          }
                        },
                        {
                          "key": "matrix_r0c0",
                          "value": {
                            "i": 256
                          }
                        },
                        {
                          "key": "matrix_r0c1",
                          "value": {
                            "i": 0
                          }
                        },
                        {
                          "key": "matrix_r0c2",
                          "value": {
                            "i": 359
                          }
                        },
                        {
                          "key": "src_image_size_w",
                          "value": {
                            "i": 250
                          }
                        },
                        {
                          "key": "crop_size_w",
                          "value": {
                            "i": 0
                          }
                        },
                        {
                          "key": "rbuv_swap_switch",
                          "value": {
                            "b": false
                          }
                        },
                        {
                          "key": "padding",
                          "value": {
                            "b": false
                          }
                        },
                        {
                          "key": "ax_swap_switch",
                          "value": {
                            "b": false
                          }
                        },
                        {
                          "key": "top_padding_size",
                          "value": {
                            "i": 0
                          }
                        },
                        {
                          "key": "matrix_r1c0",
                          "value": {
                            "i": 256
                          }
                        },
                        {
                          "key": "matrix_r1c1",
                          "value": {
                            "i": -88
                          }
                        },
                        {
                          "key": "matrix_r1c2",
                          "value": {
                            "i": -183
                          }
                        },
                        {
                          "key": "resize",
                          "value": {
                            "b": false
                          }
                        },
                        {
                          "key": "resize_output_h",
                          "value": {
                            "i": 0
                          }
                        },
                        {
                          "key": "related_input_rank",
                          "value": {
                            "i": 0
                          }
                        },
                        {
                          "key": "load_start_pos_h",
                          "value": {
                            "i": 0
                          }
                        },
                        {
                          "key": "matrix_r2c0",
                          "value": {
                            "i": 256
                          }
                        },
                        {
                          "key": "matrix_r2c1",
                          "value": {
                            "i": 454
                          }
                        },
                        {
                          "key": "matrix_r2c2",
                          "value": {
                            "i": 0
                          }
                        },
                        {
                          "key": "resize_output_w",
                          "value": {
                            "i": 0
                          }
                        },
                        {
                          "key": "var_reci_chn_0",
                          "value": {
                            "f": "1"
                          }
                        },
                        {
                          "key": "var_reci_chn_1",
                          "value": {
                            "f": "1"
                          }
                        },
                        {
                          "key": "var_reci_chn_2",
                          "value": {
                            "f": "1"
                          }
                        },
                        {
                          "key": "load_start_pos_w",
                          "value": {
                            "i": 0
                          }
                        },
                        {
                          "key": "var_reci_chn_3",
                          "value": {
                            "f": "1"
                          }
                        },
                        {
                          "key": "single_line_mode",
                          "value": {
                            "b": false
                          }
                        },
                        {
                          "key": "output_bias_0",
                          "value": {
                            "i": 16
                          }
                        },
                        {
                          "key": "output_bias_1",
                          "value": {
                            "i": 128
                          }
                        },
                        {
                          "key": "output_bias_2",
                          "value": {
                            "i": 128
                          }
                        },
                        {
                          "key": "right_padding_size",
                          "value": {
                            "i": 0
                          }
                        },
                        {
                          "key": "bottom_padding_size",
                          "value": {
                            "i": 0
                          }
                        },
                        {
                          "key": "min_chn_0",
                          "value": {
                            "f": 0
                          }
                        },
                        {
                          "key": "min_chn_1",
                          "value": {
                            "f": 0
                          }
                        },
                        {
                          "key": "min_chn_2",
                          "value": {
                            "f": 0
                          }
                        },
                        {
                          "key": "min_chn_3",
                          "value": {
                            "f": 0
                          }
                        },
                        {
                          "key": "crop",
                          "value": {
                            "b": false
                          }
                        },
                        {
                          "key": "cpadding_value",
                          "value": {
                            "f": 0
                          }
                        },
                        {
                          "key": "left_padding_size",
                          "value": {
                            "i": 0
                          }
                        }
                      ]
                    }
                  }
                }
    ```
