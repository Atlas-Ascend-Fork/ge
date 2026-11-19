# Quantized Model Fails Because the Calibration Dataset Size Does Not Match the Model Input Size

<!-- md-trans-meta sourceCommit=0f9e5e70315a2093b02dc21846074a745fd9219a translatedAt=2026-11-16T15:00:18.163Z pushedAt=2026-11-17T12:32:01.315Z nl=2 -->

## Symptoms

**When executing the ATC model conversion command, model quantization-related options (quantizing the weights in the model from float32 to int8) are configured through the --compression\_optimize\_conf parameter**, and the following error is reported:

```console
[ERROR] AMCT(21177,atc.bin):2023-04-14-14:43:17[utils_acl.cpp:133]input image size[1579014] is not equal to model input size[3158028]
[ERROR] AMCT(21177,atc.bin):2023-04-14-14:43:17[sample_process.cpp:234]memcpy device buffer failed
[ERROR] AMCT(21177,atc.bin):2023-04-14-14:43:17[sample_process.cpp:298]execute PreProcess failed
[ERROR] AMCT(21177,atc.bin):2023-04-14-14:43:17[sample_process.cpp:275]ACL model infer failed.
[ERROR] AMCT(21177,atc.bin):2023-04-14-14:43:17[quantize_api.cpp:240]sample process failed
[ERROR] AMCT(21177,atc.bin):2023-04-14-14:43:17[quantize_api.cpp:376]Do Calibration failed.
[ERROR] AMCT(21177,atc.bin):2023-04-14-14:43:20[inner_graph_calibration.cpp:78]Failed to execute InnerQuantizeGraph.
...
ATC run failed, Please check the detail log, Try 'atc --help' for more information
```

## Cause Analysis

The error message **input image size\[xxxxxx\] is not equal to model input size\[xxxxxx\]**** indicates that the calibration dataset size does not match the model input size during quantization of the quantized model. The mismatch may be caused by a discrepancy between the shape of the calibration dataset and the shape of the model input, or by a discrepancy between the data type of the calibration dataset and the data type of the model input.

## Solution

Check one by one whether the calibration dataset shape and data type are consistent with the model input shape and data type.

During quantization, the model input shape is configured through the input\_shape parameter in the quantization configuration file. The model input data type must be obtained by the user from the website where the model is obtained, or viewed by opening the model file with third-party software.
