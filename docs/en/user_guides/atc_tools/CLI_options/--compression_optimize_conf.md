# --compression\_optimize\_conf

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-22T02:50:09.693Z pushedAt=2026-10-23T07:44:15.798Z -->

## Product Support

<!-- npu="950" id2 -->
- Ascend 950PR&950DT products: Not supported
<!-- end id2 -->

<!-- npu="A3" id3 -->
- Atlas A3 products: Supported
<!-- end id3 -->

<!-- npu="910b" id4 -->
- Atlas A2 products: Supported
<!-- end id4 -->

<!-- npu="310b" id5 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id5 -->

<!-- npu="310p" id6 -->
- Atlas inference products: Supported
<!-- end id6 -->

<!-- npu="910" id7 -->
- Atlas training products: Supported
<!-- end id7 -->

<!-- @ref: ge/res/docs/zh/user_guides/atc_tools/CLI_options/--compression_optimize_conf_res.md#id1 -->

## Feature Description

Specifies the path and file name of the compression optimization configuration file. This parameter enables the compression optimization features specified in the configuration file, thereby improving network performance.

## Related Parameters

If the **calibration** quantization feature is configured through this parameter, the high-precision feature cannot be used. For example, you can no longer configure **force\_fp32** or **must\_keep\_origin\_dtype (original fp32 input)** through the [--precision\_mode](--precision_mode.md) parameter; you can no longer configure **origin** through the [--precision\_mode\_v2](--precision_mode_v2.md) parameter; and you cannot configure the **high\_precision** parameter through [--op\_precision\_mode](--op_precision_mode.md). Setting quantization parameters in high-precision mode yields neither the performance benefit of quantization nor the precision benefit of high-precision mode.

## Parameter Values

**Parameter Value:**
Path and file name of the configuration file.

**Parameter Value Format:**
Path and file name: supports uppercase and lowercase letters (a-z, A-Z), digits (0-9), underscores (\_), hyphens (-), periods (.), and Chinese characters.

**Parameter Value Constraints:**

<!-- npu="310p,310b" id9 -->
Currently, only the following two compression modes are supported. You can determine which compression mode to configure based on the actual situation:
<!-- end id9 -->

```textproto
enable_first_layer_quantization: true
calibration:
{
    input_data_dir: ./data.bin,d2.bin
    input_shape: in:16,16;in1:16,16
    config_file: simple_config.cfg
    infer_soc: xxxxxx
    infer_aicore_num: 10
    infer_device_id: 0
    infer_ip: x.x.x.x
    infer_port: 8000
    log: info
}
```

Where:

- **enable\_first\_layer\_quantization**: controls whether to optimize the first-layer convolution of AIPP (AIPP is fused with the Quant operator before the first-layer Conv2D of the quantized model). In the configuration file, the part before the colon indicates the name of the compression optimization feature, and the part after the colon indicates whether to enable the feature. true means enabled, false means disabled, and the default value is disabled.

    **When the enable\_first\_layer\_quantization** feature is enabled, performance gains can be obtained only when the network structure contains an AIPP+Conv2D structure and the [--enable\_small\_channel](--enable_small_channel.md) parameter is set to 1 in the atc command. Because the quantized model has a certain degree of precision loss, you can determine whether to enable this feature based on the actual situation.

    <!-- npu="310p,310b" id8 -->
    Only the Atlas inference products and Atlas 200I/500 A2 inference product support this feature.
    <!-- end id8 -->

    <!-- @ref: ge/res/docs/zh/user_guides/atc_tools/CLI_options/--compression_optimize_conf_res.md#id3 -->

- **calibration**: post-training quantization, which refers to quantization performed after model training is complete. It quantizes the weights in the trained model from floating-point numbers (float32/float16 are currently supported) to low-bit integers (such as int8), and calibrates and quantizes the data (activation) based on the inference process using a small amount of calibration data, thereby accelerating model inference. Post-training quantization is simple and easy to use, requires only a small amount of calibration data, and is suitable for scenarios that pursue high ease of use and lack training resources. For a sample of post-training quantization, click [Link](https://gitee.com/ascend/samples/tree/master/python/level1_single_api/9_amct/atc).

    The parameters are described as follows:

  - input\_data\_dir: Mandatory configuration. Path of the bin file of the model input calibration data. If the model has multiple inputs, separate the bin data files of the multiple inputs with commas (,). The calibration dataset is used to calculate quantization parameters. The calibration set should be representative. It is recommended to use a subset of the test set as the calibration dataset. For details about how to generate the bin file of the calibration data, see [Link](https://gitee.com/ascend/samples/blob/master/python/level1_single_api/9_amct/amct_caffe/cmd/src/process_data.py).

  - input\_shape: Mandatory configuration. Shape information of the model input calibration data, for example, input\_name1:n1,c1,h1,w1;input\_name2:n2,c2,h2,w2. Separate nodes with semicolons (;).

  - config\_file: Optional configuration. Simple configuration file for post-training quantization. For the configuration example and parameter description of this file, see [Simple Configuration File](../references/quantization_simple_config.md).

  <!-- npu="A3,910b,910,310p,310b" id11 -->
  - infer\_soc: Mandatory configuration. Name of the chip used for post-training quantization calibration inference. For details about how to query the name, see [Parameter Values](../CLI_options/--soc_version.md).

  - infer\_aicore\_num: Optional configuration. Number of AI Cores used for post-training quantization calibration inference. For details about how to query the number, see [--aicore\_num](--aicore_num.md).

  - infer\_device\_id: Optional configuration. ID of the AI processor device used for post-training quantization calibration inference. The default value is 0.
  <!-- end id11 -->

  <!-- npu="310b" id10 -->
  - infer\_ip: Mandatory for the Atlas 200I/500 A2 inference product in the Ascend RC scenario. IP address of the server where the NCS software package resides.

  - infer\_port: Mandatory for the Atlas 200I/500 A2 inference product in the Ascend RC scenario. Port of the server where the NCS software package resides.
  <!-- end id10 -->

  <!-- @ref: ge/res/docs/zh/user_guides/atc_tools/CLI_options/--compression_optimize_conf_res.md#id2 -->

  - log: Optional configuration. Sets the log level for post-training quantization. This parameter controls only the log level displayed during post-training quantization. The default level is info:

    - debug: Outputs log information at the debug/info/warning/error/event levels.
    - info: Outputs log information at the info/warning/error/event levels.
    - warning: Outputs log information at the warning/error/event levels.
    - error: Outputs log information at the error/event levels.

    In addition, the log printing to the screen and log writing to disk during post-training quantization are controlled by the **AMCT\_LOG\_DUMP** environment variable:

    - **export AMCT\_LOG\_DUMP=1**: Logs are printed to the screen and written to the amct\_log\__ timestamp _/amct\_acl.log file in the current path, and the quantization factor record file and graph file are not saved.
    - **export AMCT\_LOG\_DUMP=2**: Logs are written to the amct\_log\__ timestamp _/amct\_acl.log file in the current path, and the quantization factor record file is saved.
    - **export AMCT\_LOG\_DUMP=3**: Logs are written to the amct\_log\__ timestamp _/amct\_acl.log file in the current path, and the quantization factor record file and graph file are saved.

        To prevent the disk from being filled up by continuous writing of log files, record files, and graph files, clean up these files in a timely manner.

        If the user configures the **ASCEND\_WORK\_PATH** environment variable, the preceding logs, quantization factor record files, and graph files are stored in the path specified by this environment variable. For example, if ASCEND\_WORK\_PATH=/home/test, the storage path is /home/test/amct\_acl/amct\_log\__\{pid\}_\__ timestamp_. The amct\_acl is automatically created during model conversion, and \{pid\} is the process ID.

        > [!NOTE]
        >
        > The preceding log files, record files, and graph files are overwritten when quantization is executed again. Save them as needed. In addition, because the size of the generated log files depends on the number of layers of the model to be quantized, ensure that the server where the ATC tool resides has sufficient space:
        > For example, when quantizing the resnet101 model, if the log level is set to INFO, the log file is about 12KB and the intermediate temporary files are about 260MB; if the log level is set to DEBUG, the log file is about 390KB and the intermediate temporary files are about 430MB.

**Default value:**
None.

## Recommended Configuration and Benefits

None

## Example

Assume that the compression optimization configuration file is named compression\_optimize.cfg. The following is an example of its content:

```textproto
enable_first_layer_quantization: true
calibration:
{
    input_data_dir: ./data.bin,d2.bin
    input_shape: in:16,16;in1:16,16
    config_file: simple_config.cfg
    infer_soc: xxxxxx
    infer_aicore_num: 10
    infer_device_id: 0
    infer_ip: x.x.x.x
    infer_port: 8000
    log: info
}
```

Upload the file to the server where the ATC tool is located, for example, to $HOME/module. The following is an example of its usage:

```bash
atc --compression_optimize_conf=$HOME/module/compression_optimize.cfg ...
```

After quantization is enabled, if the message "build\_main build graph\[infer\_graph\_info\] failed" is displayed during model conversion, see [Quantization Enabled but "build\_main build graph\[infer\_graph\_info\] failed" Displayed During Model Conversion](../FAQ/quantization_build_error.md).

## Dependencies and Constraints

- To use the compression feature of this parameter, you need to install the AMCT (acl) software package separately. For details about how to obtain and install the package, see "Preparing the Environment \> Obtaining the Software Package, Uploading the Software Package, and Installing the Tool" in [*AMCT Model Compression Tool*](https://www.hiascend.com/document/detail/en/CANNCommunityEdition/920/devaids/amct/atlasamct_16_0001.html).
- When using the **enable\_first\_layer\_quantization** feature of this parameter, ensure that the model used is a deployment model output after quantization by AMCT.
<!-- npu="A3,910b,910,310p,310b" id13 -->
- When using the **calibration** post-training quantization feature in the configuration file, only installation scenarios **with an NPU device** are supported. For details, see [*CANN Software Installation*](https://www.hiascend.com/document/detail/en/CANNCommunityEdition/920/softwareinst/instg/instg_0000.html?OS=openEuler&InstallType=netyum) to set up the corresponding product environment.
<!-- end id13 -->
<!-- npu="310b" id12 -->
- For the Atlas 200I/500 A2 inference product in the Ascend RC scenario, you also need to install the NCS software on the runtime environment and configure the key certificate. For details, see [*AOE Tuning Tool*](https://www.hiascend.com/document/detail/en/CANNCommunityEdition/920/devaids/aoe/auxiliarydevtool_aoe_0001.html) > AOE Tool (Ascend RC) > Preparing the Environment.
<!-- end id12 -->

<!-- @ref: ge/res/docs/zh/user_guides/atc_tools/CLI_options/--compression_optimize_conf_res.md#id4 -->

- When using calibration in the configuration file for post-training quantization, the ATC tool calls the AMCT quantization API to perform related operations. The schematic diagram is as follows:

    **Figure 1**  Simplified diagram of post-training quantization
    ![Diagram](../figures/ptq_diagram.png "Simplified diagram of post-training quantization")
