# AIPP Configuration Example

<!-- md-trans-meta sourceCommit=8ed641509c09b657d721fdefadfbf6dee35dac94 translatedAt=2026-10-16T09:06:37.484Z pushedAt=2026-10-19T07:28:26.468Z -->

## Static AIPP Configuration Example

An AIPP configuration file supports defining multiple groups of AIPP configurations to apply different AIPP processing to different model inputs. When configuring multiple groups of AIPP parameters, place each group of AIPP configuration in one aipp\_op item. If the model has only one input, you only need to configure the first group of aipp\_op.

The following example uses a network model with multiple inputs for illustration:

> [!NOTE]
>
>- Static AIPP + dynamic shape scenario: At model conversion, if static AIPP is set through the [--insert\_op\_conf](../CLI_options/--insert_op_conf.md) parameter and dynamic shape is set through the [--input\_shape](../CLI_options/--input_shape.md) parameter, the following applies: if the model has only one input, this scenario is not supported; if the model has multiple inputs, you must configure different input nodes separately, for example, set static AIPP for one input node and dynamic shape for another node.
>- If, at model conversion, the user sets the [--dynamic\_image\_size](../CLI_options/--dynamic_image_size.md) dynamic resolution parameter, that is, the width and height of the input image are uncertain, and also sets the static AIPP function through the [--insert\_op\_conf](../CLI_options/--insert_op_conf.md) parameter: in this scenario, Crop and Padding cannot be enabled in the AIPP configuration file, and the values of src\_image\_size\_w and src\_image\_size\_h in the configuration file must be set to 0.

- Use the related\_input\_rank parameter to identify which model input is processed by AIPP. The following configuration defines two groups of AIPP parameters to process the first and second model inputs respectively:

    ```textproto
    aipp_op {
           aipp_mode : static
           related_input_rank : 0  # Identify the first input for AIPP processing
           src_image_size_w : 608
           src_image_size_h : 608
           crop : false
           input_format : YUV420SP_U8
           csc_switch : true
           rbuv_swap_switch : false
           matrix_r0c0 : 298
           matrix_r0c1 : 0
           matrix_r0c2 : 409
           matrix_r1c0 : 298
           matrix_r1c1 : -100
           matrix_r1c2 : -208
           matrix_r2c0 : 298
           matrix_r2c1 : 516
           matrix_r2c2 : 0
           input_bias_0 : 16
           input_bias_1 : 128
           input_bias_2 : 128
           mean_chn_0 : 104
           mean_chn_1 : 117
           mean_chn_2 : 123
    }
    aipp_op {
           aipp_mode : static
           related_input_rank : 1   # Identify the second input for AIPP processing
           src_image_size_w : 608
           src_image_size_h : 608
           crop : false
           input_format : YUV420SP_U8
           csc_switch : true
           rbuv_swap_switch : false
           matrix_r0c0 : 298
           matrix_r0c1 : 0
           matrix_r0c2 : 409
           matrix_r1c0 : 298
           matrix_r1c1 : -100
           matrix_r1c2 : -208
           matrix_r2c0 : 298
           matrix_r2c1 : 516
           matrix_r2c2 : 0
           input_bias_0 : 16
           input_bias_1 : 128
           input_bias_2 : 128
           mean_chn_0 : 104
           mean_chn_1 : 117
           mean_chn_2 : 123
    }
    ```

- Use the related\_input\_name parameter to identify which model input is processed by AIPP. Here you need to fill in the name of the model input (the value corresponding to input) or the output of the first-layer node of the model (the value corresponding to the top parameter). **This parameter applies only to Caffe network models** and cannot be used together with the related\_input\_rank parameter. If both are configured, related\_input\_name takes precedence over related\_input\_rank.

    The following configuration defines two groups of AIPP parameters to process the first and second model inputs respectively:

    ```textproto
    aipp_op {
           aipp_mode : static
          related_input_name : "data"  # Identify the first input for AIPP processing
           src_image_size_w : 608
           src_image_size_h : 608
           crop : false
           input_format : YUV420SP_U8
           csc_switch : true
           rbuv_swap_switch : false
           matrix_r0c0 : 298
           matrix_r0c1 : 0
           matrix_r0c2 : 409
           matrix_r1c0 : 298
           matrix_r1c1 : -100
           matrix_r1c2 : -208
           matrix_r2c0 : 298
           matrix_r2c1 : 516
           matrix_r2c2 : 0
           input_bias_0 : 16
           input_bias_1 : 128
           input_bias_2 : 128
           mean_chn_0 : 104
           mean_chn_1 : 117
           mean_chn_2 : 123
    }
    aipp_op {
           aipp_mode : static
          related_input_name : "im_info"   # Identifies that AIPP processing is performed on the second input
           src_image_size_w : 608
           src_image_size_h : 608
           crop : false
           input_format : YUV420SP_U8
           csc_switch : true
           rbuv_swap_switch : false
           matrix_r0c0 : 298
           matrix_r0c1 : 0
           matrix_r0c2 : 409
           matrix_r1c0 : 298
           matrix_r1c1 : -100
           matrix_r1c2 : -208
           matrix_r2c0 : 298
           matrix_r2c1 : 516
           matrix_r2c2 : 0
           input_bias_0 : 16
           input_bias_1 : 128
           input_bias_2 : 128
           mean_chn_0 : 104
           mean_chn_1 : 117
           mean_chn_2 : 123
    }
    ```

## Dynamic AIPP Configuration Example

An AIPP configuration file supports defining multiple AIPP configurations to apply different AIPP processing to different model inputs. When configuring multiple AIPP parameter sets, place each AIPP configuration in a separate aipp\_op item. If the model has only one input, you only need to configure the first aipp\_op.

The following example uses a multi-input network model for illustration.

### Configuration Example

> [!NOTE]
>
> - If you set the [--dynamic\_batch\_size](../CLI_options/--dynamic_batch_size.md) dynamic batch gear parameter at model conversion and also configure the dynamic AIPP function through the [--insert\_op\_conf](../CLI_options/--insert_op_conf.md) parameter, during actual inference, when you call the **aclmdlSetInputAIPP** API to set the dynamic AIPP related parameter values, ensure that batch\_size is set to the maximum batch number. For API details, see [aclmdlSetInputAIPP](../../../api/graph_engine_api/c/acl/aclmdlSetInputAIPP.md).
> - If you set the [--dynamic\_image\_size](../CLI_options/--dynamic_image_size.md) dynamic resolution parameter at model conversion and also configure the dynamic AIPP function through the [--insert\_op\_conf](../CLI_options/--insert_op_conf.md) parameter, during actual inference, when you call the **aclmdlSetInputAIPP** API to set the dynamic AIPP related parameter values, you cannot enable the Crop and Padding functions. In this scenario, you also need to ensure that the width and height set through the aclmdlSetInputAIPP API are equal to the width and height set through the **aclmdlSetDynamicHWSize** API, and both must be set to the width and height of the maximum dynamic resolution gear. For API details, see the [Model Execution](../../../api/graph_engine_api/c/acl/model_execute_APIs.md) section.
> - If you set the [--input\_shape](../CLI_options/--input_shape.md) dynamic shape range parameter at model conversion and also configure the AIPP function through the [--insert\_op\_conf](../CLI_options/--insert_op_conf.md) parameter, the width and height of the AIPP output must be within the range set by [--input\_shape](../CLI_options/--input_shape.md).

In the dynamic AIPP scenario, you do not need to manually configure parameters such as csc\_switch and rbuv\_swap\_switch. After configuring the related parameters according to the following configuration file, ATC adds a model input (hereinafter referred to as AippData) for dynamic AIPP at model conversion.

During actual inference, you need to call the **aclmdlSetInputAIPP** API to set the dynamic AIPP related parameter values and then pass them to the newly added AippData. The structure constructed by AippData based on the passed parameter values is [Input Structure of Dynamic AIPP Parameters](#input-structure-of-dynamic-aipp-parameters), which does not need to be manually processed. For API details, see [aclmdlSetInputAIPP](../../../api/graph_engine_api/c/acl/aclmdlSetInputAIPP.md).

```textproto
aipp_op
{
    aipp_mode: dynamic
    related_input_rank: 0       # Identify that AIPP processing is performed on the first input
    max_src_image_size: 752640  # Maximum size of the input image. This parameter is mandatory.
}
aipp_op
{
    aipp_mode: dynamic
    related_input_rank: 1         # Identify that AIPP processing is performed on the second input
    max_src_image_size: 752640    # Maximum size of the input image. This parameter is mandatory.
}
```

### Input Structure of Dynamic AIPP Parameters

After the dynamic AIPP file is configured according to the [Configuration Example](#configuration-example), when a new model input (AippData) is added for dynamic AIPP during model inference and parameter values are passed in, the following structure is automatically formed. This structure does not need to be manually processed by users:

```c++
typedef struct tagAippDynamicBatchPara
{
    int8_t cropSwitch;              //crop switch
    int8_t scfSwitch;               //resize switch
    int8_t paddingSwitch;   // 0: unable padding,
                           // 1: padding config value,sfr_filling_hblank_ch0 ~    sfr_filling_hblank_ch2
                          // 2: padding source picture data, single row/column copy
                          // 3: padding source picture data, block copy
                          // 4: padding source picture data, mirror copy
    int8_t rotateSwitch;  //rotate switch, 0: non-rotate, 1: rotate 90°clockwise, 2: rotate 180°clockwise, 3: rotate 270° clockwise
    int8_t reserve[4];
    int32_t cropStartPosW;          //the start horizontal position of cropping
    int32_t cropStartPosH;          //the start vertical position of cropping
    int32_t cropSizeW;              //crop width
    int32_t cropSizeH;              //crop height
    int32_t scfInputSizeW;          //input width of scf
    int32_t scfInputSizeH;          //input height of scf
    int32_t scfOutputSizeW;         //output width of scf
    int32_t scfOutputSizeH;         //output height of scf
    int32_t paddingSizeTop;         //top padding size
    int32_t paddingSizeBottom;      //bottom padding size
    int32_t paddingSizeLeft;        //left padding size
    int32_t paddingSizeRight;       //right padding size
    int16_t dtcPixelMeanChn0;       //mean value of channel 0
    int16_t dtcPixelMeanChn1;       //mean value of channel 1
    int16_t dtcPixelMeanChn2;       //mean value of channel 2
    int16_t dtcPixelMeanChn3;       //mean value of channel 3
    uint16_t dtcPixelMinChn0;       //min value of channel 0
    uint16_t dtcPixelMinChn1;       //min value of channel 1
    uint16_t dtcPixelMinChn2;       //min value of channel 2
    uint16_t dtcPixelMinChn3;       //min value of channel 3
    uint16_t dtcPixelVarReciChn0;   //sfr_dtc_pixel_variance_reci_ch0
    uint16_t dtcPixelVarReciChn1;   //sfr_dtc_pixel_variance_reci_ch1
    uint16_t dtcPixelVarReciChn2;   //sfr_dtc_pixel_variance_reci_ch2
    uint16_t dtcPixelVarReciChn3;   //sfr_dtc_pixel_variance_reci_ch3
    int8_t reserve1[16];            //32B assign, for ub copy
}kAippDynamicBatchPara;
typedef struct tagAippDynamicPara
{
    uint8_t inputFormat;        //input format: YUV420SP_U8/XRGB8888_U8/RGB888_U8
    //uint8_t outDataType; //output data type: CC_DATA_HALF,CC_DATA_INT8, CC_DATA_UINT8
    int8_t cscSwitch;               //csc switch
    int8_t rbuvSwapSwitch;          //rb/uv swap switch
    int8_t axSwapSwitch;            //RGBA->ARGB, YUVA->AYUV swap switch
    int8_t batchNum;                //batch parameter number
    int8_t reserve1[3];
    int32_t srcImageSizeW;          //source image width
    int32_t srcImageSizeH;          //source image height
    int16_t cscMatrixR0C0;          //csc_matrix_r0_c0
    int16_t cscMatrixR0C1;          //csc_matrix_r0_c1
    int16_t cscMatrixR0C2;          //csc_matrix_r0_c2
    int16_t cscMatrixR1C0;          //csc_matrix_r1_c0
    int16_t cscMatrixR1C1;          //csc_matrix_r1_c1
    int16_t cscMatrixR1C2;          //csc_matrix_r1_c2
    int16_t cscMatrixR2C0;          //csc_matrix_r2_c0
    int16_t cscMatrixR2C1;          //csc_matrix_r2_c1
    int16_t cscMatrixR2C2;          //csc_matrix_r2_c2
    int16_t reserve2[3];
    uint8_t cscOutputBiasR0;   //output bias for RGB to YUV, element of row 0, unsigned number
    uint8_t cscOutputBiasR1;   //output bias for RGB to YUV, element of row 1, unsigned number
    uint8_t cscOutputBiasR2;   //output bias for RGB to YUV, element of row 2, unsigned number
    uint8_t cscInputBiasR0;    //input bias for YUV to RGB, element of row 0, unsigned number
    uint8_t cscInputBiasR1;    //input bias for YUV to RGB, element of row 1, unsigned number
    uint8_t cscInputBiasR2;    //input bias for YUV to RGB, element of row 2, unsigned number
    uint8_t reserve3[2];
    int8_t reserve4[16];            //32B assign, for ub copy
    kAippDynamicBatchPara aippBatchPara;  //allow transfer several batch para.
} kAippDynamicPara;
```
