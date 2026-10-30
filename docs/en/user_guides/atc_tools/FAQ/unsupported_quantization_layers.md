# Why Does Model Quantization Fail When the Model Contains Layers That Do Not Support Quantization?

<!-- md-trans-meta sourceCommit=0f9e5e70315a2093b02dc21846074a745fd9219a translatedAt=2026-10-27T14:24:47.709Z pushedAt=2026-10-28T08:56:26.016Z nl=2 -->

## Symptoms

**When executing the ATC model conversion command, model quantization-related options (quantizing the weights in the model from float32 to int8) are configured through the --compression\_optimize\_conf parameter**, and the following error is reported:

```console
ATC start working now, please wait for a moment.
[ERROR][ProcessScale][52] Not support scale greater than 1 / FLT_EPSILON.
[ERROR][WtsArqCalibrationCpuKernel][188] ArqQuantCPU scale is illegal.
[ERROR][ArqQuant][301] WtsArqCalibrationCpuKernel of format CO_CI_KH_KW failed.
[ERROR] AMCT(14815,atc.bin):2023-04-14-12:23:19[weight_algorithm.cpp:137]Default/network-DeepLabV3/resnet-Resnet/layer4-SequentialCell/0-Bottleneck/downsample-SequentialCell/0-Conv2d/Conv2D-op311 arq weight fake quant failed!
[ERROR] AMCT(14815,atc.bin):2023-04-14-12:23:19[weight_calibration_pass.cpp:90]Fail to execute WeightFakeQuant without trans!
[ERROR] AMCT(14815,atc.bin):2023-04-14-12:23:19[weight_calibration_pass.cpp:185]layer Default/network-DeepLabV3/resnet-Resnet/layer4-SequentialCell/0-Bottleneck/downsample-SequentialCell/0-Conv2d/Conv2D-op311 run WeightFakeQuantArq failed
[ERROR] AMCT(14815,atc.bin):2023-04-14-12:23:19[graph_optimizer.cpp:43]pass run failed
[ERROR] AMCT(14815,atc.bin):2023-04-14-12:23:19[quantize_api.cpp:227]Do GenerateCalibrationGraph optimizer pass failed.
[ERROR] AMCT(14815,atc.bin):2023-04-14-12:23:19[quantize_api.cpp:363]Generate calibration Graph failed.
[ERROR] AMCT(14815,atc.bin):2023-04-14-12:23:22[inner_graph_calibration.cpp:78]Failed to execute InnerQuantizeGraph.
```

## Cause Analysis

The error message **layer  _xxxxxx_  run WeightFakeQuantArq failed** indicates that the current model contains weight-related layers that do not support quantization. These layers need to be skipped.

## Solution

Skip the layers that do not support quantization. The configuration method is as follows:

1. Add a configuration to skip the layers that do not support quantization.

    Create a configuration file with the .cfg extension, for example _simple\_config.cfg_. The file content is as follows. The bold part indicates the layers that do not support quantization as shown in the error message:

    ```ini
    skip_layers: "Default/network-DeepLabV3/resnet-Resnet/layer4-SequentialCell/0-Bottleneck/downsample-SequentialCell/0-Conv2d/Conv2D-op311"
    ```

    In addition, add the config\_file parameter to the quantization configuration file specified by the --compression\_optimize\_conf parameter:

    ```textproto
    calibration:
    {
        input_data_dir: xxxxxx
        config_file: simple_config.cfg
        input_shape: xxxxxx
        infer_soc: xxxxxx
    }
    ```

2. Re-execute model conversion.
3. Re-execute inference.

    If skipping the layers that do not support quantization affects the result data of model inference, you need to adjust the model by yourself and then quantize the model again.
