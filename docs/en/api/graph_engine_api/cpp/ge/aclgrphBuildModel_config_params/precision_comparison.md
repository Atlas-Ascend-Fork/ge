# Precision Comparison

<!-- md-trans-meta sourceCommit=03b8f43b8d6d016735ca55a32b59cd73bc779bda translatedAt=2026-11-17T02:53:49.509Z pushedAt=2026-11-18T02:41:08.901Z nl=2 -->

## QUANT\_DUMPABLE

Whether to collect dump data of quantization operators. The actual **options** parameter corresponding to this parameter is `ge.quant_dumpable`.

When performing precision locating by referring to the "Model Inference \> Precision/Performance Optimization \> Suggestions for Improving Model Inference Precision" section in [Application Development](https://gitcode.com/cann/docs/blob/master/docs/zh/app-dev/c/00_acl_cpp_dev.md), if an AMCT-quantized model exists, the graph compilation process may optimize the inputs and outputs of quantization operators when the model is converted into an offline om model, thereby affecting the export of dump data of quantization operators. For example, for two quantized convolution computations, the intermediate output is optimized into an int8 quantized output.

To address this, the **QUANT\_DUMPABLE** parameter is introduced. After this parameter is configured, the inputs and outputs of quantization operators are not fused, and transdata operators are inserted to restore the format of the original model, so that the dump data of quantization operators can be collected.

**Parameter values:**

- **0**: (Default value) The graph compilation process may optimize the inputs and outputs of quantization operators, in which case the dump data of quantization operators cannot be obtained.
- **1**: After this configuration is enabled, the dump data of quantization operators can be collected.

**Configuration example:**

```c++
{ge::ir_option::QUANT_DUMPABLE, "1"}
```

**Usage constraints:**
When [OFFLINE_MODE](basic_functions.md#offline_mode) is set to 7, this parameter cannot be configured.

**Applicable Products:**

Supported on all chips.
