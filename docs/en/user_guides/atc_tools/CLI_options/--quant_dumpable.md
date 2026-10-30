# --quant\_dumpable

<!-- md-trans-meta sourceCommit=bd3de52c8a808742e9c01b524792105f46cad44c translatedAt=2026-10-22T03:36:50.574Z pushedAt=2026-10-27T01:50:42.972Z -->

## Product Support

Supported on all chips.

## Feature Description

Whether to collect dump data of quantization operators.

When performing precision locating by referring to the "Model Inference \> Precision/Performance Optimization \> Model Inference Precision Improvement Suggestions" section in *[Application Development](https://gitcode.com/cann/docs/blob/9.2.0/docs/zh/app-dev/c/00_acl_cpp_dev.md)*, if there is a model quantized by AMCT, when the model is converted into an om offline model, the graph compilation process may optimize the inputs and outputs of quantization operators, thereby affecting the export of dump data of quantization operators. For example, for two quantized convolution computations, the intermediate output is optimized into the int8 quantized output.

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
