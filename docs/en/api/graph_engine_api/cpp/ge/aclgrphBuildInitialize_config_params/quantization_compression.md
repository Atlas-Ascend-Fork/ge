# Quantization Compression

<!-- npu="950,A3,910b,910,310p,310b" id8 -->
<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-10-16T11:43:52.228Z pushedAt=2026-10-28T07:40:22.202Z nl=2 -->

## COMPRESSION\_OPTIMIZE\_CONF

Path and file name of the compression optimization feature configuration file. This parameter enables the compression optimization features specified in the configuration file to improve network performance. For example: **/home/test/compression\_optimize.cfg**. The actual **options** parameter corresponding to this parameter is `ge.compressionOptimizeConf`.

**Configuration example:**

```c++
{ge::ir_option::COMPRESSION_OPTIMIZE_CONF, "/home/test/compression_optimize.cfg"}
```

The following is an example of the file content configuration:

```text
enable_first_layer_quantization:true
```

- The current configuration file supports only the **enable\_first\_layer\_quantization** feature, which controls whether the first-layer convolution of AIPP is optimized (AIPP is fused with the **Quant** operator before the first-layer convolution **CONV2D** of the quantized model).

    **When the enable\_first\_layer\_quantization** feature is enabled, performance gains are possible only when the network structure contains an AIPP+CONV2D structure and the **ENABLE\_SMALL\_CHANNEL** parameter is set to 1 during model compilation. Because the quantized model has a certain degree of precision loss, users decide whether to enable this feature based on actual conditions.

- In the configuration file, the part before the colon indicates the name of the compression optimization feature, and the part after the colon indicates whether to enable the feature. **true** means enabled, **false** means disabled, and the default is disabled.

**Applicable products:**

<!-- npu="950" id1 -->
- Ascend 950PR&950DT series products: Not supported
<!-- end id1 -->
<!-- npu="A3" id2 -->
- Atlas A3 series products: Not supported
<!-- end id2 -->
<!-- npu="910b" id3 -->
- Atlas A2 products: Not supported
<!-- end id3 -->
<!-- npu="310b" id4 -->
- Atlas 200I/500 A2 inference products: Supported
<!-- end id4 -->
<!-- npu="310p" id5 -->
- Atlas inference products: Supported
<!-- end id5 -->
<!-- npu="910" id6 -->
- Atlas training products: Not supported
<!-- end id6 -->

<!-- end id8 -->

## SPARSITY

Enables the global sparsity feature. The actual option corresponding to this parameter is `ge.enableSparseMatrixWeight`.

For a model output by AMCT (Ascend Model Compression Toolkit) after 4-to-2 structured sparsity, there may be scenarios where at least 2 of every 4 consecutive Cin dimension elements of the weight are 0. During model conversion, enabling the global sparsity switch filters the elements in such scenarios down to 2, thereby reducing the computation workload of subsequent inference and improving inference performance.

Due to hardware constraints, this parameter cannot be used together with **ENABLE\_COMPRESS\_WEIGHT** or **COMPRESS\_WEIGHT\_CONF**.

**Parameter values:**

- 1: Enables 4-to-2 structured sparsity.
- 0: (Default value) Disables the sparsity feature.

**Configuration example:**

```c++
{ge::ir_option::SPARSITY, "1"}
```

**Constraints:**
When using this parameter, ensure that the model is a sparse model. It is recommended that you obtain it using the combined compression feature of AMCT (TensorFlow) or AMCT (PyTorch), and the combined compression can only be in the 4-to-2 structured sparsity + quantization-aware training mode.

**Applicable products:**

<!-- npu="950" id9 -->
- Ascend 950PR&950DT series products: Not supported
<!-- end id9 -->
<!-- npu="A3" id10 -->
- Atlas A3 series products: Supported
<!-- end id10 -->
<!-- npu="910b" id11 -->
- Atlas A2 series products: Supported
<!-- end id11 -->
<!-- npu="310b" id12 -->
- Atlas 200I/500 A2 inference products: Supported
<!-- end id12 -->
<!-- npu="310p" id13 -->
- Atlas inference products: Not supported
<!-- end id13 -->
<!-- npu="910" id14 -->
- Atlas training products: Not supported
<!-- end id14 -->
<!-- @ref: ge/res/docs/en/api/graph_engine_api/cpp/ge/aclgrphBuildInitialize_config_params/quantization_compression_res.md#id1 -->
