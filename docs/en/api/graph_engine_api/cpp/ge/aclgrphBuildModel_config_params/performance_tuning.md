# Performance Tuning

<!-- md-trans-meta sourceCommit=9d821ee607484f54716ece49d9dbdbe1c4f9524d translatedAt=2026-10-16T11:57:25.778Z pushedAt=2026-10-28T07:40:22.220Z nl=2 -->

## OP\_PRECISION\_MODE

Sets the precision mode used inside the operator during processing, supporting one or more operators. By passing a custom precision mode configuration file op\_precision.ini through this parameter, you can set different precision modes for different operators. The **options** parameter that this parameter actually corresponds to is `ge.exec.op_precision_mode`.

**Configuration example:**

```c++
{ge::ir_option::OP_PRECISION_MODE, "op_precision.ini"}
```

The configuration file supports setting the following precision modes:

- **high\_precision**: Indicates high precision.
- **high\_performance**: Indicates high performance.
<!-- npu="A3,910b" id1 -->
- **enable\_float\_32\_execution**: Uses the FP32 data type inside the operator during processing. In this scenario, the FP32 data type is not automatically converted to the HF32 data type. If the precision loss exceeds expectations when using HF32 for computation, you can enable this configuration to specify that some operators use FP32 for internal computation to maintain precision.

    **This option is supported only on the following product types:**

    <!-- npu="A3" id2 -->
    Atlas A3 series products
    <!-- end id2 -->

    <!-- npu="910b" id3 -->
    Atlas A2 series products
    <!-- end id3 -->

<!-- end id1 -->

<!-- npu="A3,910b" id4 -->
- **enable\_hi\_float\_32\_execution**: Uses the HF32 data type inside the operator. After this configuration is enabled, the FP32 data type is automatically converted to the HF32 data type. This mechanism reduces the space occupied by data and improves performance.

    **This option is not supported in the current version.**
<!-- end id4 -->

- **support\_out\_of\_bound\_index**: Performs out-of-bounds validation on the indices input of gather, scatter, and segment operators. The validation reduces the execution performance of the operators.
- **keep\_fp16**: Uses the FP16 data type inside the operator. In this scenario, the FP16 data type is not automatically converted to the FP32 data type. If the performance does not meet expectations when FP32 is used for computation and the precision requirement is not high, you can select the keep\_fp16 mode, which **sacrifices precision to improve performance. This low-precision mode is not recommended.**
- **super\_performance**: Indicates ultra-high performance. Compared with high performance, the algorithm computation formula is optimized.

To view the precision/performance mode values supported by a specific operator, see the **opp/built-in/op_impl/ai_core/tbe/impl_mode/all_ops_impl_mode.ini** file in the file storage path after the CANN software is installed.

**The following is an example**: In the ini file, the precision mode is set by operator type and node name, with one operator type or node name precision mode per line. **Setting the precision mode by node name has a higher priority than setting it by operator type.**

```text
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

**Constraints:**

- This parameter cannot be used together with the **OP\_SELECT\_IMPL\_MODE** and **OPTYPELIST\_FOR\_IMPLMODE** parameters. If the three parameters are configured at the same time, only the mode specified by the **OP\_PRECISION\_MODE** parameter takes effect.
<!-- npu="A3,910b" id5 -->
- For the same operator, if **enable\_hi\_float\_32\_execution** or **enable\_float\_32\_execution** is configured through the **OP\_PRECISION\_MODE** parameter, it is not recommended to use it together with the **ALLOW\_HF32** parameter in this scenario. If they are used together, the priority is as follows:

    op\_precision\_mode\(ByNodeName, set the precision mode by node name\) \> allow\_hf32 \> op\_precision\_mode\(ByOpType, set the precision mode by operator type\)
<!-- end id5 -->

- This parameter is not recommended. If the network performance or precision is not optimal when using the high-performance or high-precision mode, you can use this parameter to adjust the precision mode of a specific operator by configuring the ini file.

**Applicable products:**

Supported on all chips.

<!-- npu="950,A3,910b,910,310p,310b" id6 -->
## TILING\_SCHEDULE\_OPTIMIZE

Switch for Tiling offload scheduling optimization. The actual **options** parameter corresponding to this parameter is `ge.tiling_schedule_optimize`.

Because the internal storage of the AI Core on the NPU cannot fully accommodate all input and output data of an operator, a portion of the input data must be moved in for computation and then moved out, followed by moving in the next portion of input data for computation. This process is called Tiling. The computation program that determines the parameters related to the data splitting algorithm (such as the block size moved each time and the total number of loops) based on information such as the operator's shape is called the Tiling implementation. Since the Tiling implementation performs only scalar computation, which the AI Core is not good at, it is generally executed on the CPU on the Host side. However, the Tiling implementation is offloaded to the Device side for execution when the following conditions are met:

1. The model has a static shape.
2. The operators in the model support Tiling offload, such as fusion operators like **FusedInferAttentionScore** and **IncreFlashAttention**.
3. The values of operators that support Tiling offload have dependencies, and the value of the preceding operator must have an execution result on the device. If the dependent value is a **Const**, Tiling does not need to be offloaded for execution, and Tiling is completed at compile time.

**Parameter values:**

- **0**: Disables Tiling offload. The default value is **0**.
- **1**: Enables Tiling offload.

**Configuration example:**

```c++
{ge::ir_option::TILING_SCHEDULE_OPTIMIZE, "1"}
```

**Applicable products:**

<!-- npu="950" id7 -->
- Ascend 950PR&950DT products: Supported
<!-- end id7 -->
<!-- npu="A3" id8 -->
- Atlas A3 products: Supported
<!-- end id8 -->
<!-- npu="910b" id9 -->
- Atlas A2 products: Supported
<!-- end id9 -->
<!-- npu="310b" id10 -->
- Atlas 200I/500 A2 inference products: Not supported
<!-- end id10 -->
<!-- npu="310p" id11 -->
- Atlas inference products: Supported
<!-- end id11 -->
<!-- npu="910" id12 -->
- Atlas training products: Not supported
<!-- end id12 -->
<!-- end id6 -->
