# Quick Start

<!-- md-trans-meta sourceCommit=ba12e592e8f84bc59709464bd3ad6ca010498d8b translatedAt=2026-11-16T15:05:37.052Z pushedAt=2026-11-17T12:48:01.732Z nl=2 -->

This chapter uses model conversion under each framework as an example to demonstrate how to quickly convert an offline model.

> [!NOTE] Note
>
> - **Version compatibility notes:**
>   <!-- npu="950,A3,910b,910,310p,310b" id6 -->
>   - **For 8.5.0 and later versions**: When using the ATC tool for model conversion, the ops operator package matching the target AI processor must be installed. Otherwise, compilation fails.
>   <!-- end id6 -->
>   - An offline model converted in a lower-version CANN software package environment can run in a higher-version CANN software package environment, with compatibility across four version cycles.
>   <!-- npu="950,A3,910b,910,310p,310b" id1 -->
> - If the development environment architecture is Arm (aarch64) and model conversion takes a long time, see [Model Conversion Takes a Long Time When the Development Environment Architecture Is Arm (aarch64)](../FAQ/arm_aarch64_conversion_slow.md) for a solution.
>   <!-- end id1 -->
> - If the user sets the network model precision parameter [--precision\_mode](../CLI_options/--precision_mode.md) or [--precision\_mode\_v2](../CLI_options/--precision_mode_v2.md) during model conversion:
>   - By default, both parameters prioritize performance, which may cause precision overflow during subsequent inference. If a precision issue occurs during inference, see "Model Inference \> Precision/Performance Optimization \> Suggestions for Improving Model Inference Precision" in *[Application Development](https://gitcode.com/cann/docs/blob/master/docs/en/app-dev/c/00_acl_cpp_dev.md)* for troubleshooting.
>   - If the user focuses on precision issues, other values can be set, for example, set --precision\_mode to must\_keep\_origin\_dtype or set --precision\_mode\_v2 to origin.

## Converting a TensorFlow Network Model from an Open-Source Framework into an Offline Model

1. Obtain the TensorFlow network model.

    Click [Link](https://gitee.com/ascend/ModelZoo-TensorFlow/tree/master/TensorFlow/contrib/cv/resnet50_for_TensorFlow) and follow the on-page instructions to obtain the model file (\*.pb) of the ResNet50 network. Then, as a user running the CANN software package, upload the obtained file to any directory in the development environment, for example, the $HOME/module/ directory.

2. Execute the following command to generate an offline model. (The directories and files used in the following command are examples. Please use the actual ones.)

    ```bash
    atc --model=$HOME/module/resnet50_tensorflow_1.7.pb --framework=3 --output=$HOME/module/out/tf_resnet50 --soc_version=<soc_version>
    ```

    - --model: Path of the ResNet50 network model file.
    - --framework: original framework type. 3 indicates TensorFlow.
    - --output: path of the generated offline model.
    - --soc\_version: model of the AI processor.

    For details about the parameters, see [Parameter Description](../CLI_options/README.md). Use the _<soc\_version\>_ value corresponding to the chip name for model conversion, and then perform inference. For details about how to query the chip, see [--soc\_version](../CLI_options/--soc_version.md).

3. If the following information is displayed, the model conversion is successful.

    ```console
    ATC run success, welcome to the next use.
    ```

    After the command is successfully executed, you can view the offline model (for example, tf\_resnet50.om) in the path specified by the --output parameter.
    <!-- npu="950,A3,910b,910,310p,310b" id2 -->
    If the model conversion fails, see *[Fault Handling](https://gitcode.com/cann/docs/blob/master/docs/en/troubleshooting/00_troubleshooting.md)*\>\"Error Code Reference\" for auxiliary troubleshooting.
    <!-- end id2 -->

## Converting an ONNX Network Model into an Offline Model

1. Obtain the ONNX network model.

    Click [Link](https://gitcode.com/Ascend/ModelZoo-PyTorch/tree/master/ACL_PyTorch/built-in/cv/Resnet50_Pytorch_Infer) to go to the ModelZoo page, view the "Quick Start \> Model Inference" section in README.md to obtain the \*.onnx model file, and then upload the obtained file to any directory in the development environment as a user running the CANN software package, for example, to the $HOME/module/ directory.

2. Execute the following command to generate an offline model. (The directories and files used in the following command are examples. Please use the actual ones.)

    ```bash
    atc --model=$HOME/module/resnet50_official.onnx --framework=5 --output=$HOME/module/out/onnx_resnet50 --soc_version=<soc_version>
    ```

    - --model: Path of the ResNet50 network model file.
    - --framework: type of the original framework. 5 indicates ONNX.
    - --output: path of the generated offline model.
    - --soc\_version: model of the AI processor.

    For details about the parameters, see [Parameter Description](../CLI_options/README.md). Use the _<soc\_version\>_ value corresponding to the chip name for model conversion, and then perform inference. For details about how to query the chip, see [--soc\_version](../CLI_options/--soc_version.md).

3. If the following information is displayed, the model conversion is successful.

    ```console
    ATC run success, welcome to the next use.
    ```

    After the command is successfully executed, you can view the offline model (for example, onnx\_resnet50.om) in the path specified by the --output parameter.
    <!-- npu="950,A3,910b,910,310p,310b" id3 -->
    If the model conversion fails, see *[Fault Handling](https://gitcode.com/cann/docs/blob/master/docs/en/troubleshooting/00_troubleshooting.md)*\>\"Error Code Reference\" for auxiliary troubleshooting.
    <!-- end id3 -->

## Converting a Caffe Network Model of an Open Source Framework into an Offline Model

1. Obtain the Caffe network model.

    You can obtain the model file (\*.prototxt) and weight file (\*.caffemodel) of the ResNet50 network from the following links, and upload the obtained files to any directory in the development environment as a user who runs the CANN software package, for example, to the $HOME/module/ directory.

    - Model file of the ResNet50 network (\*.prototxt): Click [Link](https://obs-9be7.obs.cn-east-2.myhuaweicloud.com/003_Atc_Models/AE/ATC%20Model/resnet50/resnet50.prototxt) to download the file.
    - Weight file of the ResNet50 network (\*.caffemodel): Click [Link](https://obs-9be7.obs.cn-east-2.myhuaweicloud.com/003_Atc_Models/AE/ATC%20Model/resnet50/resnet50.caffemodel) to download the file.

2. Execute the following command to generate an offline model. (The directories and files used in the following command are examples. Please use the actual ones.)

    ```bash
    atc --model=$HOME/module/resnet50.prototxt --weight=$HOME/module/resnet50.caffemodel --framework=0 --output=$HOME/module/out/caffe_resnet50 --soc_version=<soc_version>
    ```

    - --model: Path of the ResNet50 network model file.
    - --weight: Path of the ResNet50 network weight file.
    - --framework: Type of the original framework. 0 indicates Caffe.
    - --output: path of the generated offline model.
    - --soc\_version: model of the AI processor.

    For details about the parameters, see [Parameter Description](../CLI_options/README.md). Use the _<soc\_version\>_ value corresponding to the chip name for model conversion, and then perform inference. For details about how to query the chip, see [--soc\_version](../CLI_options/--soc_version.md).

3. If the following information is displayed, the model conversion is successful.

    ```console
    ATC run success, welcome to the next use.
    ```

    After the command is successfully executed, you can view the offline model (for example, caffe\_resnet50.om) in the path specified by the --output parameter.
    <!-- npu="950,A3,910b,910,310p,310b" id4 -->
    If the model conversion fails, see *[Fault Handling](https://gitcode.com/cann/docs/blob/master/docs/en/troubleshooting/00_troubleshooting.md)*\>\"Error Code Reference\" for auxiliary troubleshooting.
    <!-- end id4 -->

## Converting a Model File in \*.air Format into an Offline Model

1. Obtain a model file in \*.air format.

    Click [Link](https://obs-9be7.obs.cn-east-2.myhuaweicloud.com:443/cannInfo/model/resnet50_export.air) to obtain the model file of the ResNet50 network, and upload the obtained file to any path in the development environment as the user who runs the CANN software package, for example, the $HOME/module/ directory.

2. Execute the following command to generate an offline model. (The directories and files used in the following command are examples. Please use the actual ones.)

    ```bash
    atc --model=$HOME/module/resnet50_export.air --framework=1 --output=$HOME/module/out/resnet50_air --soc_version=<soc_version>
    ```

    - --model: path of the model file in \*.air format.
    - --framework: type of the original framework. 1 indicates a model file in \*.air format.
    - --output: path of the generated offline model.
    - --soc\_version: model of the AI processor.

    For details about the parameters, see [Parameter Description](../CLI_options/README.md). Use the _<soc\_version\>_ value corresponding to the chip name for model conversion, and then perform inference. For details about how to query the chip, see [--soc\_version](../CLI_options/--soc_version.md).

3. If the following information is displayed, the model conversion is successful.

    ```console
    ATC run success, welcome to the next use.
    ```

    After the command is successfully executed, you can view the offline model (for example, _resnet50_\_air.om) in the path specified by the --output parameter.
    <!-- npu="950,A3,910b,910,310p,310b" id5 -->
    If the model conversion fails, see *[Fault Handling](https://gitcode.com/cann/docs/blob/master/docs/en/troubleshooting/00_troubleshooting.md)*\>\"Error Code Reference\" for auxiliary troubleshooting.
    <!-- end id5 -->
