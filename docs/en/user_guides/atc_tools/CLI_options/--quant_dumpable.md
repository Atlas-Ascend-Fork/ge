# --quant\_dumpable

<!-- md-trans-meta sourceCommit=becb235792507fcdd0382168301e8d0fb82e3246 translatedAt=2026-11-16T14:31:59.203Z pushedAt=2026-11-17T11:54:24.073Z nl=2 -->

## Product Support

Supported on all chips.

## Function Description

Whether to collect dump data of quantization operators.

When locating precision issues by referring to the "Model Inference \> Precision/Performance Optimization \> Model Inference Precision Improvement Suggestions" section in *[Application Development](https://gitcode.com/cann/docs/blob/master/docs/en/app-dev/c/00_acl_cpp_dev.md)*, if a model quantized by AMCT exists, the input and output of the quantized operator may be optimized during graph compilation when the model is converted into an om offline model, thereby affecting the export of the quantized operator's dump data. For example, the intermediate output of two quantized convolution computations is optimized into an int8 quantized output.

To address this issue, the --quant\_dumpable parameter is introduced. After this parameter is configured, the inputs and outputs of quantization operators are not fused, and a transdata operator is inserted to restore the format of the original model, so that the dump data of quantization operators can be collected.

## Parameter Values

- 0: (Default value) During graph compilation, the input and output of quantization operators may be optimized, in which case the dump data of quantization operators cannot be obtained.
- 1: After this configuration is enabled, the dump data of quantization operators can be collected.

## Recommended Configuration and Benefits

When Data Dump is enabled, it is recommended that this parameter be set to 1 to ensure that the dump data of quantization operators can be collected.

## Example

```bash
atc --quant_dumpable=1 ...
```
