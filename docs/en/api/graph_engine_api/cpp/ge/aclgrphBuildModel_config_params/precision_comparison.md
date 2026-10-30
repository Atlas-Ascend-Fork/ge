# Precision Comparison

<!-- md-trans-meta sourceCommit=ba12e592e8f84bc59709464bd3ad6ca010498d8b translatedAt=2026-10-16T11:59:21.653Z pushedAt=2026-10-28T07:40:22.217Z nl=2 -->

## QUANT\_DUMPABLE

Whether to collect dump data of quantization operators. The actual **options** parameter corresponding to this parameter is `ge.quant_dumpable`.

When performing precision locating by referring to the "Model Inference \> Precision/Performance Optimization \> Suggestions for Improving Model Inference Precision" section in [Application Development](https://gitcode.com/cann/docs/blob/9.2.0/docs/zh/app-dev/c/00_acl_cpp_dev.md), if an AMCT-quantized model exists, the graph compilation process may optimize the inputs and outputs of quantization operators when the model is converted into an offline om model, thereby affecting the export of dump data of quantization operators. For example, for two quantized convolution computations, the intermediate output is optimized into an int8 quantized output.

To address this, the **QUANT\_DUMPABLE** parameter is introduced. After this parameter is configured, the inputs and outputs of quantization operators are not fused, and transdata operators are inserted to restore the format of the original model, so that the dump data of quantization operators can be collected.

**Parameter values:**

- **0**: (Default value) The graph compilation process may optimize the inputs and outputs of quantization operators, in which case the dump data of quantization operators cannot be obtained.
- **1**: After this configuration is enabled, the dump data of quantization operators can be collected.

**Configuration example:**

```c++
{ge::ir_option::QUANT_DUMPABLE, "1"}
```

**Applicable Products:**

Supported on all chips.
