# Quantization Simplified Configuration File

<!-- md-trans-meta sourceCommit=3b4c8f106b74c3a75512dd245c2b55b143c90db0 translatedAt=2026-10-27T14:32:34.303Z pushedAt=2026-10-28T10:35:05.558Z nl=2 -->

To automatically control the quantization process, for example, to control which layers are quantized and which quantization algorithm is used, you can implement it through the cfg configuration file constructed in this chapter.

## Configuration File Parameters

The messages supported by the configuration file and the corresponding parameter descriptions are as follows:

- AMCTConfig: simplified configuration for AMCT post-training quantization.

    | Required | Type | Field | Description |
    | --- | --- | --- | --- |
    | optional | bool | activation_offset | Whether data quantization uses an offset. Global configuration parameter.<br><br>  - With offset: (default) data quantization uses asymmetric quantization.<br>  - Without offset: data quantization uses symmetric quantization. |
    | optional | bool | joint_quant | Whether to perform Eltwise joint quantization. The default value is false, indicating that joint quantization is disabled.<br>After it is enabled, some networks may have improved performance but degraded accuracy. |
    | repeated | string | skip_layers | Names of layers that do not need to be quantized. |
    | repeated | string | skip_layer_types | Types of layers that do not need to be quantized. |
    | optional | int32 | version | Version of the simplified configuration file. |
    | optional | CalibrationConfig | common_config | Common quantization configuration, that is, global quantization configuration parameters. If a layer is not overridden by override_layer_types or override_layer_configs, this configuration is used.<br>Parameter priority: override_layer_configs > override_layer_types > common_config |
    | repeated | OverrideLayerType | override_layer_types | Overrides the quantization configuration of a certain type of layers, that is, performs differentiated quantization on certain layers.<br>For example, if the quantization factor search step configured in the global quantization configuration parameters is 0.01, you can use this parameter to perform differentiated quantization on some layers and set the search step to 0.02.<br>Parameter priority: override_layer_configs > override_layer_types > common_config |
    | repeated | OverrideLayer | override_layer_configs | Overrides the quantization configuration of a certain layer, that is, performs differentiated quantization on certain layers.<br>For example, if the quantization factor search step configured in the global quantization configuration parameters is 0.01, you can use this parameter to perform differentiated quantization on some layers and set the search step to 0.02.<br>Parameter priority: override_layer_configs > override_layer_types > common_config |
    | optional | bool | do_fusion | Whether to enable BN fusion. The default value is true, indicating that this function is enabled. |
    | repeated | string | skip_fusion_layers | Layers that skip BN fusion. After configuration, BN fusion is not performed on these layers. |
    | repeated | TensorQuantize | tensor_quantize | Performs post-training quantization on the input tensors of specified nodes in the network model to improve inference performance during data transfer.<br>Currently, tensor quantization is supported only for MaxPool, Add, and Eltwise operators. |

- OverrideLayerType: name of a layer type that supports quantization.

    | Required | Type | Field | Description |
    | --- | --- | --- | --- |
    | required | string | layer_type | Name of a layer type that supports quantization. |
    | required | CalibrationConfig | calibration_config | Quantization configuration to be reset. |

- OverrideLayer: resets the quantization configuration of a layer.

    | Required | Type | Field | Description |
    | --- | --- | --- | --- |
    | required | string | layer_name | Name of the layer whose configuration is reset. |
    | required | CalibrationConfig | calibration_config | Quantization configuration to be reset. |

- CalibrationConfig: configuration for Calibration quantization.

    | Required | Type | Field | Description |
    | --- | --- | --- | --- |
    | - | ARQuantize | arq_quantize | Weight quantization algorithm configuration.<br>arq_quantize: ARQ quantization algorithm configuration. |
    | - | FMRQuantize | ifmr_quantize | Data quantization algorithm configuration.<br>ifmr_quantize: IFMR quantization algorithm configuration. |
    | optional | bool | weight_compress_only | Whether to perform weight quantization only. In the weight-only quantization scenario, the supported data types must be Float32 and Float16.<br><br>  - true: performs weight quantization only.<br>  - false: quantizes both weights and data. The default value is false. |

- ARQuantize: ARQ weight quantization algorithm configuration.

    | Required | Type | Field | Description |
    | --- | --- | --- | --- |
    | optional | bool | channel_wise | Whether to use a different quantization factor for each channel.<br><br>  - true: Each channel is quantized independently with a different quantization factor.<br>  - false: All channels are quantized together and share a quantization factor. |
    | optional | bool | asymmetric | Whether to apply asymmetric quantization to weights. It is used to control the selection of the per-layer quantization algorithm.<br>It takes effect only when weight_compress_only is set to true. If weight_compress_only is set to false, asymmetric can only be set to false.<br><br>  - true: Weight quantization uses asymmetric quantization (offset is not 0).<br>  - false: Weight quantization uses symmetric quantization (offset is 0). Default: false.<br><br>If this parameter is configured in override_layer_configs, override_layer_types, and common_config, the effective priority is as follows:<br>override_layer_configs > override_layer_types > common_config |
    | optional | uint32 | quant_bits | Weight quantization bit width. INT6, INT7, and INT8 are supported. Default: INT8 quantization.<br>When this field is set to INT6 or INT7, only Conv2d operators are supported.<br>If quant_bits is set to INT6 or INT7 in common_config, it takes effect only for Conv2d operators, and other operators use the default INT8.<br>For ONNX network models, if quant_bits is set to INT6 or INT7 for Conv operators in override_layer_types, it takes effect only when the weight dim is 4. |

- FMRQuantize: FMR data quantization algorithm configuration.

    | Required | Type | Field | Description |
    | --- | --- | --- | --- |
    | optional | float | search_range_start | Left boundary of the quantization factor search range. |
    | optional | float | search_range_end | Right boundary of the quantization factor search range. |
    | optional | float | search_step | Quantization factor search step. |
    | optional | float | max_percentile | Maximum value search position. |
    | optional | float | min_percentile | Minimum value search position. |
    | optional | bool | asymmetric | Whether to apply asymmetric quantization to data. It is used to control the selection of the per-layer quantization algorithm.<br><br>  - true: Asymmetric quantization<br>  - false: Symmetric quantization<br><br>If this parameter is configured in override_layer_configs, override_layer_types, and common_config, or if the<br>activation_offset parameter is configured, the effective priority is as follows:<br>override_layer_configs > override_layer_types > common_config > activation_offset |
    | optional | CalibrationDataType | dst_type | Quantization bit width, indicating whether INT8 or INT16 quantization is used for data quantization. Default: INT8 quantization.<br>In the current version, only INT8 quantization is supported. |

- TensorQuantize: Configuration of the input tensors that require post-training quantization.

    | Required | Type | Field | Description |
    | --- | --- | --- | --- |
    | required | string | layer_name | Name of the node whose input tensors require post-training quantization. Currently, only the input tensors of MaxPool operators can be quantized. |
    | required | uint32 | input_index | Input index of the node whose input tensors require post-training quantization. |
    | - | FMRQuantize | ifmr_quantize | Data quantization algorithm configuration.<br>ifmr_quantize: IFMR quantization algorithm configuration. Default: IFMR quantization algorithm. |

## Configuration Examples

- A sample **uniform quantization simple configuration file** quant.cfg constructed based on this file is as follows: Optype must be configured as the operator type defined based on Ascend IR. For details about the mapping, see [Layers and Constraints Supported by Quantization](quantization_layers_constraints.md).

    ```textproto
    # global quantize parameter
    activation_offset : true
    joint_quant : false
    enable_auto_nuq : false
    version : 1
    skip_layers : "Optype"
    skip_layer_types: "Optype"
    do_fusion: true
    skip_fusion_layers : "Optype"
    common_config : {
        arq_quantize : {
            channel_wise : true
            quant_bits : 7

        }
        ifmr_quantize : {
            search_range_start : 0.7
            search_range_end : 1.3
            search_step : 0.01
            max_percentile : 0.999999
            min_percentile : 0.999999
            asymmetric : true
        }
    }

    override_layer_types : {
        layer_type : "Optype"
        calibration_config : {
            arq_quantize : {
                channel_wise : false
            }
            ifmr_quantize : {
                search_range_start : 0.8
                search_range_end : 1.2
                search_step : 0.02
                max_percentile : 0.999999
                min_percentile : 0.999999
                asymmetric : false
            }
        }
    }

    override_layer_configs : {
        layer_name : "Opname"
        calibration_config : {
            arq_quantize : {
                channel_wise : true
            }
            ifmr_quantize : {
                search_range_start : 0.8
                search_range_end : 1.2
                search_step : 0.02
                max_percentile : 0.999999
                min_percentile : 0.999999
                asymmetric : false
            }
        }
    }
    tensor_quantize {
        layer_name: "Opname"
        input_index: 0
        ifmr_quantize: {
            search_range_start : 0.7
            search_range_end : 1.3
            search_step : 0.01
            min_percentile : 0.999999
            asymmetric : false
           }
    }
    tensor_quantize {
        layer_name: "Opname"
        input_index: 0
    }
    ```

- A sample **weight-only quantization simple configuration file** _quant_.cfg constructed based on this file:

    ```textproto
    activation_offset : true
    joint_quant : false
    version : 1
    do_fusion: true
    common_config : {
       weight_compress_only : true
        arq_quantize : {
            channel_wise : true
            asymmetric : false

        }
    }

    override_layer_types : {
        layer_type : "Optype"
        calibration_config : {
            weight_compress_only : true
            arq_quantize : {
                channel_wise : true
                asymmetric : true
                quant_bits : 6
            }
        }
    }

    override_layer_configs : {
        layer_name : "Opname"
        calibration_config : {
            weight_compress_only : true
            arq_quantize : {
                channel_wise : true
                asymmetric : true
            }
        }
    }
    ```
