# Performance Tuning

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-10-26T12:54:25.358Z pushedAt=2026-10-28T07:42:19.754Z nl=2 -->

## ge.exec.op\_precision\_mode

Sets the precision mode for internal processing of specified operators, supporting one or more operators. By passing a custom precision mode configuration file op\_precision.ini through this parameter, you can set different precision modes for different operators.

In the ini file, precision modes are set by operator type and node name. Each line sets the precision mode for one operator type or node name. Setting the precision mode by node name takes precedence over setting it by operator type.

The configuration file supports the following precision modes:

- high\_precision: Indicates high precision.
- high\_performance: Indicates high performance.
- enable\_float\_32\_execution: Uses the FP32 data type for processing inside the operator. In this scenario, the FP32 data type is not automatically converted to the HF32 data type. If the precision loss exceeds expectations when using HF32 for computation, you can enable this configuration to specify that some operators use FP32 for computation inside the operator to maintain precision.

    <!-- npu="950,A3,910b" id1 -->
    **This option is supported only on the following product types:**

    <!-- npu="950" id6 -->
    Ascend 950PR&950DT products
    <!-- end id6 -->

    <!-- npu="910b" id7 -->
    Atlas A2 products
    <!-- end id7 -->

    <!-- npu="A3" id8 -->
    Atlas A3 products
    <!-- end id8 -->
    <!-- end id1 -->

- enable\_hi\_float\_32\_execution: Uses the HF32 data type inside the operator. After this configuration is enabled, the FP32 data type is automatically converted to the HF32 data type. This mechanism reduces the space occupied by data and improves performance.

    **This option is not supported in the current version.**

- support\_out\_of\_bound\_index: Performs out-of-bounds check on the indices input of gather, scatter, and segment operators. The check reduces operator execution performance.
- keep\_fp16: Uses the FP16 data type inside the operator. In this scenario, the FP16 data type is not automatically converted to the FP32 data type. If FP32 computation does not meet the expected performance and the precision requirement is not high, you can select the keep\_fp16 mode. **This mode sacrifices precision for performance and is not recommended.**
- super\_performance: Indicates ultra-high performance. Compared with high performance, the algorithm formula is optimized.

For the precision/performance mode values supported by a specific operator, see the opp/built-in/op\_impl/ai\_core/tbe/impl\_mode/all\_ops\_impl\_mode.ini file in the file storage path after CANN software installation.

op\_precision.ini configuration example:

```ini
[ByOpType]
optype1=high_precision
optype2=high_performance
optype3=enable_hi_float_32_execution
optype4=support_out_of_bound_index

[ByNodeName]
nodename1=high_precision
nodename2=high_performance
nodename3=enable_hi_float_32_execution
nodename4=support_out_of_bound_index
```

**Configuration example:**

```c++
{"ge.exec.op_precision_mode", "$HOME/conf/op_precision.ini"};
```

**Required/Optional**: Optional

**Effective level**: Global

## ge.exec.variable\_acc

Whether to enable variable format optimization.

**Parameter values:**

- True: (default) Enable.
- False: Disable.

To improve training efficiency, during variable initialization in network execution, variables are converted into a data format more suitable for running on the AI processor. However, in special user-required scenarios, you can disable this function switch.

**Usage constraints:**

When enabled, ge.AllowMultiGraphParallelCompile cannot be set to "1" at the same time; otherwise, a validation error is reported.

**Configuration example:**

```c++
{"ge.exec.variable_acc", "True"};
```

**Required/Optional**: Optional

**Effective level**: global/session/graph

## ge.graphMaxParallelModelNum

In graph execution mode, the same graph can be loaded and executed in parallel by multiple models on the same device. This parameter controls the maximum number of models allowed to be loaded in parallel.

**Parameter values:**

1\~INT32\_MAX, defaulting to 8.

**Configuration example:**

```c++
{"ge.graphMaxParallelModelNum", "8"};
```

**Required/Optional**: Optional

**Effective level**: Global/session/graph

## ge.tiling\_schedule\_optimize

Switch for optimizing Tiling offload scheduling.

Because the internal storage of the AI Core in the NPU cannot fully hold all input and output data of an operator, a portion of the input data must be moved in for computation and then moved out each time, followed by moving in the next portion of input data. This process is called Tiling. The computation program that determines the data partitioning algorithm parameters (such as the block size moved each time and the total number of loops) based on information such as the operator's shape is called the Tiling implementation. Since the Tiling implementation performs only scalar computation, which the AI Core is not proficient at, it is generally executed on the host-side CPU. However, the Tiling implementation is offloaded to the device side for execution when the following conditions are met:

1. The model has a static shape.
2. The operators in the model support Tiling offload, such as fused operators like FusedInferAttentionScore and IncreFlashAttention.
3. Operators that support Tiling offload have value dependencies, and the value of the preceding operator must be an execution result on the device. If the dependent value is a Const, Tiling does not need to be offloaded for execution, and Tiling is completed at compile time.

<!-- npu="A3,910b,310p" id2 -->
This parameter is supported only on the following products:

<!-- npu="910b" id3 -->
Atlas A2 products
<!-- end id3 -->

<!-- npu="A3" id4 -->
Atlas A3 products
<!-- end id4 -->

<!-- npu="310p" id5 -->
Atlas inference products
<!-- end id5 -->
<!-- end id2 -->

**Parameter values:**

- 0: (Default) Disable Tiling offload.
- 1: Enable Tiling offload.

**Configuration example:**

```c++
{"ge.tiling_schedule_optimize", "0"};
```

**Required/Optional**: Optional

**Effective level**: Global/session/graph
