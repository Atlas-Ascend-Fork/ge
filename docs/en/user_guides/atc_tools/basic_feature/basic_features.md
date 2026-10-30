# Basic Functions

<!-- md-trans-meta sourceCommit=6910bdd915e3528be24078d698c2568ed03b113b translatedAt=2026-10-22T02:44:49.351Z pushedAt=2026-10-22T12:31:30.421Z -->

## Converting an Original Model File or Offline Model to a JSON File

### Scenario Description

If it is inconvenient for users to view the parameter information of an original model or offline model, they can convert the original model or offline model into a JSON file for viewing.

### Conversion Method

This section uses the ResNet50 network model of the TensorFlow framework as an example. Click [Link](https://gitee.com/ascend/ModelZoo-TensorFlow/tree/master/TensorFlow/contrib/cv/resnet50_for_TensorFlow) and follow the page prompts to obtain the ResNet50 network model file (\*.pb).

- Converting an original model file to a JSON file

    The command example is as follows:

    ```bash
    atc --mode=1 --om=$HOME/module/resnet50_tensorflow_1.7.pb  --json=$HOME/module/out/tf_resnet50.json  --framework=3
    ```

  - --mode: running mode. 1 indicates converting an original model file or offline model to JSON. Here it specifically refers to converting an original model file to JSON.
  - --om: specifies the path of the **ResNet50 network model file**.
  - --json: path and file name of the file converted to JSON format.
  - --framework: original framework type. 3 indicates TensorFlow.

- Converting an offline model to a JSON file

    This scenario requires that you have already obtained the om offline model file by following [Converting a TensorFlow Network Model of an Open-Source Framework to an Offline Model](../overview/quick_start.md#converting-a-tensorflow-network-model-from-an-open-source-framework-into-an-offline-model). The command example is as follows:

    ```bash
    atc --mode=1 --om=$HOME/module/out/tf_resnet50.om  --json=$HOME/module/out/tf_resnet50.json
    ```

  - --mode: running mode. 1 indicates converting an original model file or offline model to JSON. Here it specifically indicates converting an offline model file to JSON.
  - --om: specifies the path of the **offline model file**.

For details about the parameters, see [Parameter Description](../CLI_options/README.md). If the following information is displayed, the conversion is successful.
<!-- npu="950,A3,910b,910,310p,310b" id2 -->
If the model conversion fails, see *[Fault Handling](https://gitcode.com/cann/docs/blob/9.2.0/docs/zh/troubleshooting/00_troubleshooting.md)*\>"Error Code Reference" for auxiliary location.

<!-- end id2 -->
```console
ATC run success, welcome to the next use.
```

After the command is successfully executed, you can view the converted JSON file information under the path specified by the --json parameter. The following is part of the JSON snippet:

```json
{
  "node": [
    {
      "attr": [
        {
          "key": "shape",
          "value": {
            "shape": {
              "dim": [
                {
                  "size": 1
                },
                {
                  "size": 224
                },
                {
                  "size": 224
                },
                {
                  "size": 3
                }
              ]
            }
          }
        },
        {
          "key": "dtype",
          "value": {
            "type": "DT_FLOAT"
          }
        }
      ],
      "name": "Placeholder",
      "op": "Placeholder"
    },
...

  ]
}
```

## Offline Model Supports Dynamic batch\_size/ Dynamic Resolution

### Scenario Description

In some inference scenarios, for example, a target recognition network is executed after targets are detected, the input batch\_size of the target recognition network is not fixed because the number of targets is not fixed. If each inference is performed based on the maximum batch\_size or the maximum resolution, computing resources are wasted.

To address this, the ATC tool provides the [--dynamic\_batch\_size](../CLI_options/--dynamic_batch_size.md) parameter to set batch\_size levels and the [--dynamic\_image\_size](../CLI_options/--dynamic_image_size.md) parameter to set resolution levels.

### Conversion Method

The following conversion example uses the ResNet50 network model of the TensorFlow framework as an example. Click [Link](https://gitee.com/ascend/ModelZoo-TensorFlow/tree/master/TensorFlow/contrib/cv/resnet50_for_TensorFlow) and follow the page prompt to obtain the model file of the ResNet50 network (\*.pb).

1. Log in to the development environment as the CANN software package run user, and upload the model file (\*.pb) to any path in the development environment, for example, to the $HOME/module/ directory.
2. Execute the following command to generate an offline model. (The directories and files used in the following command are examples. Please use the actual ones.)

    - Dynamic batch\_size

        ```bash
        atc --model=$HOME/module/resnet50_tensorflow_1.7.pb  --framework=3 --output=$HOME/module/out/tf_resnet50 --soc_version=<soc_version> --input_shape="Placeholder:-1,224,224,3"  --dynamic_batch_size="1,2,4,8"
        ```

    - Dynamic resolution

        ```bash
        atc --model=$HOME/module/resnet50_tensorflow_1.7.pb  --framework=3 --output=$HOME/module/out/tf_resnet50 --soc_version=<soc_version>  --input_shape="Placeholder:1,-1,-1,3"  --dynamic_image_size="224,224;448,448"
        ```

    The key parameters are explained as follows:

    - --dynamic\_batch\_size: sets the dynamic batch\_size parameter.
    - --dynamic\_image\_size: sets the dynamic resolution parameter of the input image.
    - --input\_shape: specifies the shape of the model input data, used together with the --dynamic\_batch\_size or --dynamic\_image\_size parameter.
    - --model: Path of the ResNet50 network model file.
    - --framework: Type of the original framework. 3 indicates TensorFlow.

    For details about the parameters and their usage, see [Parameter Description](../CLI_options/README.md). If the following information is displayed, the model conversion is successful.
    <!-- npu="950,A3,910b,910,310p,310b" id3 -->
    If the model conversion fails, see *[Fault Handling](https://gitcode.com/cann/docs/blob/9.2.0/docs/zh/troubleshooting/00_troubleshooting.md)*\>"Error Code Reference" for auxiliary location.
    <!-- end id3 -->

    ```console
    ATC run success, welcome to the next use.
    ```

    After the command is successfully executed, you can view the offline model (for example, tf\_resnet50.om) in the path specified by the --output parameter.

    After the model conversion is complete, a new input is added to the generated OM offline model (for example, the Data input in the red box in [Figure 1](#fig1)). During model inference, the specific batch\_size value (or resolution value) is provided through this new input. For example, if the batch\_size of input a is dynamic (or the resolution is dynamic), in the OM offline model, there is an input b corresponding to a to describe the batch\_size (or resolution value) of a.

    **Figure 1**  Offline model with the dynamic batch\_size function<a id="fig1"></a>
    ![](../figures/offline_model_with_dynamic_batch_size.png "Offline model with the dynamic batch_size function")

## Offline Model Supports Dynamic Dimensions

### Scenario Description

To support scenarios where the input Tensor dimensions of network models such as Transformer are uncertain, the ATC tool provides the [--dynamic\_dims](../CLI_options/--dynamic_dims.md) parameter to implement gear setting for arbitrary dimensions in ND format. ND indicates that any format is supported.

### Conversion Method

This section uses the ResNet50 network model of the TensorFlow framework as an example. Click [Link](https://gitee.com/ascend/ModelZoo-TensorFlow/tree/master/TensorFlow/contrib/cv/resnet50_for_TensorFlow) and follow the page prompts to obtain the ResNet50 network model file (\*.pb).

1. Log in to the development environment as the user who runs the CANN software package, and upload the model file to any path in the development environment, for example, to the $HOME/module/ directory.
2. Execute the following command to generate an offline model. (The directories and files used in the following command are examples. Use the actual ones.)

    ```bash
    atc --model=$HOME/module/resnet50_tensorflow_1.7.pb --framework=3 --output=$HOME/module/out/tf_resnet50 --soc_version=<soc_version>  --input_shape="Placeholder:-1,-1,-1,3" --dynamic_dims="1,224,224;8,448,448" --input_format=ND
    ```

    The key parameters are explained as follows:

    - --dynamic\_dims: Sets the dynamic dimension gear in ND format.
    - --input\_shape: Specifies the shape of the model input data, used together with the --dynamic\_dims parameter.
    - --input\_format: Specifies the format as ND.
    - --model: Path of the ResNet50 network model file.
    - --framework: Type of the original framework. 3 indicates TensorFlow.

    For details about the parameters, see [Parameter Description](../CLI_options/README.md). If the following information is displayed, the model conversion is successful.
    <!-- npu="950,A3,910b,910,310p,310b" id4 -->
    If model conversion fails, see *[Fault Handling](https://gitcode.com/cann/docs/blob/9.2.0/docs/zh/troubleshooting/00_troubleshooting.md)*\>"Error Code Reference" for auxiliary location.
    <!-- end id4 -->

    ```console
    ATC run success, welcome to the next use.
    ```

    After the command is successfully executed, you can view the offline model under the path specified by the --output parameter.

    After model conversion is complete, a new input is added to the generated om offline model (for example, the Data input in the red box in [Figure 1](#fig2)). During model inference, the specific dimension values are provided through this new input. For example, if the dimension of input a is dynamic, the om offline model contains an input b corresponding to a to describe the dimension values of a.

    **Figure 1**  Offline model with dynamic dimensions<a id="fig2"></a>
    ![](../figures/offline_model_with_dynamic_dims.png "Offline model with dynamic dimensions")

## Customizing Input and Output Data Types of an Offline Model

### Scenario Description

During model conversion, you can specify key parameters such as the DataType and Format of the input and output nodes of the network, and the precision selection supported by model conversion.

For example, for the ResNet-50 network model in the TensorFlow framework, suppose the input data of the converted offline model must be of the Float16 type, the MaxPoolWithArgmax operator is specified as the output operator (the corresponding node name is fp32\_vars/MaxPoolWithArgmax), and the data type of this output node is specified as FP16. In this scenario, you need to use the [--input\_fp16\_nodes](../CLI_options/--input_fp16_nodes.md), [--out\_nodes](../CLI_options/--out_nodes.md), and [--output\_type](../CLI_options/--output_type.md) parameters respectively to implement the preceding functions.

### Conversion Method

This section uses the ResNet50 network model of the TensorFlow framework as an example. Click [Link](https://gitee.com/ascend/ModelZoo-TensorFlow/tree/master/TensorFlow/contrib/cv/resnet50_for_TensorFlow) and follow the page prompt to obtain the ResNet50 network model file (\*.pb).

1. Log in to the development environment as the CANN software package running user, and upload the model file (\*.pb) to any path in the development environment, for example, to the $HOME/module/ directory.
2. Execute the following command to generate the offline model. (The directories and files used in the following command are examples. Please use the actual ones.)

    ```bash
    atc --model=$HOME/module/resnet50_tensorflow_1.7.pb  --framework=3 --output=$HOME/module/out/tf_resnet50 --soc_version=<soc_version>  --input_fp16_nodes="Placeholder" --out_nodes="fp32_vars/MaxPoolWithArgmax:0" --output_type="fp32_vars/MaxPoolWithArgmax:0:FP16"
    ```

    The key parameters are explained as follows:

    - --input\_fp16\_nodes: specifies the input data type as Float16.
    - --out\_nodes: specifies the MaxPoolWithArgmax operator as the model output.
    - --output\_type: specifies the data type of the output node as Float16.
    - --model: path of the ResNet50 network model file.
    - --framework: original framework type. 3 indicates TensorFlow.

    For details about the parameters, see [Parameter Description](../CLI_options/README.md). If the following information is displayed, the model conversion is successful.
    <!-- npu="950,A3,910b,910,310p,310b" id5 -->
    If the model conversion fails, see *[Fault Handling](https://gitcode.com/cann/docs/blob/9.2.0/docs/zh/troubleshooting/00_troubleshooting.md)*\>"Error Code Reference" for auxiliary location.
    <!-- end id5 -->

    ```console
    ATC run success, welcome to the next use.
    ```

    After the command is successfully executed, you can view the offline model (for example, tf\_resnet50.om) under the path specified by the output parameter. [Figure 1](#fig3) shows the MaxPoolWithArgmax operator used as the model output operator (the following figure is opened using the Netron visualization software).

    **Figure 1**  Specify an operator as the offline model output<a id="fig3"></a>
    ![](../figures/specify_op_as_offline_model_output.png "Specify an operator as the offline model output")

## View the Software Base Version Number Using an Offline Model

### Scenario Description

Different CANN software package versions may produce offline models with different functions due to software function differences. In this scenario, you are advised to use the ATC tool that matches the CANN software version to perform model conversion again. If you already have a converted offline model and want to view the base version number of the CANN software package used, refer to this section.

### Viewing Method

1. Obtain the converted offline model, for example tf\_resnet50.om, and upload it to any directory in the development environment as the CANN software package running user, for example, to the $HOME/module/ directory.
2. Convert the offline model to a JSON file:

    ```bash
    atc --mode=1 --om=$HOME/module/tf_resnet50.om  --json=$HOME/module/out/tf_resnet50.json
    ```

    - --om: specifies the path of the **offline model file **tf\_resnet50.om.
    - --json: specifies the path and name of the file to be converted to JSON format.

    In the converted JSON file, you can view the base version used when the original model was converted to the offline model. The following is an example (a partial JSON snippet), where <version\> shows the version information:

    ```json
       {
          "key": "opp_version",
          "value": {
            "s": "<version>"
          }
        },
        ... ...
        {
          "key": "atc_version",
          "value": {
            "s": "<version>"
          }
        },
        ... ...
        {
           "key": "atc_cmdline",
           "value": {
             "s": "xxx/atc.bin --model ./resnet50_tensorflow.pb  --framework 3 --output ./out/tf_resnet50 --soc_version <soc_version>"
           }
         },
        ... ...
        {
          "key": "soc_version",
          "value": {
            "s": "<soc_version>"
           }
         },
    ```
