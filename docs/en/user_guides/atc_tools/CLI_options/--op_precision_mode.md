# --op\_precision\_mode

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-22T03:30:13.602Z pushedAt=2026-10-27T11:44:35.571Z -->

## Product Support

Supported on all chips

## Function Description

Sets the precision mode for processing inside the operator. You can specify one or more operators.

## Associated Parameters

- This parameter cannot be used together with the [--op\_select\_implmode](--op_select_implmode.md) and [--optypelist\_for\_implmode](--optypelist_for_implmode.md) parameters. If all three parameters are configured at the same time, only the mode specified by the [--op\_precision\_mode](--op_precision_mode.md) parameter takes effect.
<!-- npu="A3,910b" id4 -->
- For the same operator, if enable\_hi\_float\_32\_execution or enable\_float\_32\_execution is configured through the [--op\_precision\_mode](--op_precision_mode.md) parameter, it is not recommended to use it together with the [--allow\_hf32](--allow_hf32.md) parameter in this scenario. If they are used together, the priority is as follows:

    op\_precision\_mode\(ByNodeName, set the precision mode by node name \) \> allow\_hf32 \> op\_precision\_mode\(ByOpType, set the precision mode by operator type \)
<!-- end id4 -->

The associated parameter diagram is shown in [Figure 1](#fig1).

**Figure 1** Associated parameter diagram<a id="fig1"></a>
![Diagram](../figures/related_params.png "Associated parameter diagram")

In the scenario of setting the precision mode for specific operators:

1. First read the [--op\_precision\_mode](--op_precision_mode.md) parameter and verify whether the ini configuration file of this parameter exists. If it exists, parse the file and read the precision mode of the operator; otherwise, report an exception.
2. If [--op\_precision\_mode](--op_precision_mode.md) does not exist, read the [--op\_select\_implmode](--op_select_implmode.md) parameter:
    1. First check whether it is configured as the high\_xxx\_for\_all parameter. If so, parse the high\_xxx\_for\_all.ini file and read the precision mode of the operator.
    2. If high\_xxx is configured as the parameter, check whether the [--optypelist\_for\_implmode](--optypelist_for_implmode.md) parameter is configured. If yes, read the operator precision mode configured by this parameter; otherwise, parse the high\_xxx.ini file and read the operator precision mode.

## Parameter Value

**Parameter value:**
Path and file name of the configuration file (.ini format) for setting the operator precision mode. The configuration file supports the following precision modes:

- high\_precision: indicates high precision.
- high\_performance: indicates high performance.
<!-- npu="950,A3,910b" id5 -->
- enable\_float\_32\_execution: uses the FP32 data type inside the operator. In this scenario, the FP32 data type is not automatically converted to the HF32 data type. If the precision loss exceeds expectations when using HF32 for computation, you can enable this configuration to specify that some operators use FP32 for internal computation to maintain precision.

    **This option is supported only on the following product types:**

    <!-- npu="950" id1 -->
    Ascend 950PR&950DT products
    <!-- end id1 -->

    <!-- npu="910b" id2 -->
    Atlas A2 products
    <!-- end id2 -->

    <!-- npu="A3" id3 -->
    Atlas A3 products
    <!-- end id3 -->
<!-- end id5 -->

<!-- npu="950,A3,910b" id6 -->
- enable\_hi\_float\_32\_execution: uses the HF32 data type inside the operator. After this configuration is enabled, the FP32 data type is automatically converted to the HF32 data type. This mechanism reduces the space occupied by data and improves performance. **This option is not supported in the current version.**
<!-- end id6 -->

- support\_out\_of\_bound\_index: Indicates that out-of-bounds check is performed on the indices input of gather, scatter, and segment operators. The check reduces operator execution performance.
- keep\_fp16: Indicates that the FP16 data type is used inside the operator. In this scenario, the FP16 data type is not automatically converted to the FP32 data type. If the performance does not meet expectations when FP32 is used for computation and the precision requirement is not high, you can select the keep\_fp16 mode, which **sacrifices precision for performance. This low-precision mode is not recommended**.
- super\_performance: Indicates ultra-high performance. Compared with high performance, the algorithm calculation formula is optimized.

To view the precision/performance mode values supported by a specific operator, see the opp/built-in/op\_impl/ai\_core/tbe/impl\_mode/all\_ops\_impl\_mode.ini file in the file storage path after CANN software installation.

**Parameter value format:**
Path and file name: Uppercase and lowercase letters (a-z, A-Z), digits (0-9), underscores (\_), hyphens (-), periods (.), and Chinese characters are supported.

**Parameter value constraints**:

- Currently, operator precision can be set only through the .ini configuration file. The content in the configuration file is presented in key-value (operator type = precision mode) form, with one operator precision mode set on each line.
- The operator type must be the OpType of the operator defined based on Ascend IR. For details about how to view the operator type, see [How Do I Determine the Correspondence Between Operators in the Original Framework Network Model and Operators Supported by AI Processors](../FAQ/operator_correspondence_guide.md).

## Recommended Configuration and Benefits

- This parameter is not recommended. If the network performance or precision is not optimal when the high-performance or high-precision mode is used, you can use this parameter to adjust the precision mode of a specific operator by configuring an ini file.
- For the ini configuration file loaded through this parameter, you are advised to use the ini configuration file saved by the user through the [--op\_select\_implmode](--op_select_implmode.md) parameter. For details, see [Recommended Configuration and Benefits](--op_select_implmode.md#recommended-configuration-and-benefits).

## Example

Create an operator precision mode configuration file _op\_precision.ini_, and set the precision mode in the file by operator type and node name. Set the precision mode for one operator type or node name on each line. **The precision mode set by node name takes precedence over that set by operator type.**

The following is a configuration example:

<!-- npu="950,A3,910b,910,310p,310b" id7 -->
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
<!-- end id7 -->

<!-- @ref: ge/res/docs/zh/user_guides/atc_tools/CLI_options/--op_precision_mode_res.md#id1 -->

Upload the configured _op\_precision.ini_ file to any directory on the server where the ATC tool is located, for example, $HOME/conf. The following is an example:

```bash
atc --op_precision_mode=$HOME/conf/op_precision.ini ...
```

## Dependency Constraints

None.
