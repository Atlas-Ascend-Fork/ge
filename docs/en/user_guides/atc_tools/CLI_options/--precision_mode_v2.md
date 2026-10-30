# --precision\_mode\_v2

<!-- md-trans-meta sourceCommit=bd3de52c8a808742e9c01b524792105f46cad44c translatedAt=2026-10-22T03:35:38.316Z pushedAt=2026-10-27T12:06:33.442Z -->

## Product Support

Supported on all chips

## Feature Description

Sets the precision mode of the network model.

## Related Parameters

- This parameter cannot be used together with the [--precision\_mode](--precision_mode.md) parameter. It is recommended to use the [--precision\_mode\_v2](--precision_mode_v2.md) parameter, which is newly added in the new version and has clearer option value semantics for easier understanding.
- When the value is **mixed\_float16**, **mixed\_bfloat16**, or **mixed\_hif8**, if you want to make adjustments based on the built-in optimization strategy and specify which operators allow precision reduction and which do not, see the [--modify\_mixlist](--modify_mixlist.md) parameter settings.
- In inference scenarios, using the [--precision\_mode\_v2](--precision_mode_v2.md) parameter to set the precision mode of the entire network model may cause performance or precision issues for individual operators. In this case, you can use the [--keep\_dtype](--keep_dtype.md) parameter to keep the computation precision of individual operators unchanged during compilation of the original network model. However, when the [--precision\_mode\_v2](--precision_mode_v2.md) parameter is set to **origin**, the [--keep\_dtype](--keep_dtype.md) parameter does not take effect.

## Parameter Values

- **fp16 (default):**

    Indicates that when the operator precision in the original graph is float16, bfloat16, or float32, float16 is force-selected.

- **origin:**

    Keeps the precision of the original graph.

  - If the precision of an operator in the original graph is float16, and the implementation of this operator on the AI Core does not support float16 but supports only float32 and bfloat16, the system automatically uses the higher-precision float32.
  - If the precision of an operator in the original graph is float16, and the implementation of this operator on the AI Core does not support float16 but supports only bfloat16, the float16 AI CPU operator is used. If the AI CPU operator is also unsupported, an error is reported during execution.
  - If the precision of an operator in the original graph is float32, and the implementation of this operator on the AI Core does not support float32 but supports only float16, the float32 AI CPU operator is used. If the AI CPU operator is also unsupported, an error is reported during execution.

- **cube\_fp16in\_fp32out:**

    When the operator on the AI Core supports both float32 and float16 data types, the system selects different processing methods based on the operator type.

  - For matrix computation operators, the system internally processes them based on the support status of the operator implementation:
        1. Preferentially select float16 as the input data type and float32 as the output data type;
        2. If the scenario in 1 is not supported, select float32 as the input data type and float32 as the output data type;
        3. If the scenario in 2 is not supported, select float16 as the input data type and float16 as the output data type;
        4. If the scenario in 3 is not supported, an error is reported.

  - For vector computation operators, it indicates that the operator precision in the original graph is float16 or bfloat16, and float32 is force selected.

      If some operators exist in the original graph whose implementations on the AI Core do not support float32, for example, an operator supports only float16, this parameter does not take effect and the supported float16 is still used. If the implementation of the operator on the AI Core does not support float32 and a blocklist is also configured (precision\_reduce = false), the float32 AI CPU operator is used. If the AI CPU operator is also not supported, an error is reported during execution.

- **mixed\_float16:**

    Indicates that mixed precision float16, bfloat16, and float32 data types are used to process the neural network. For operators with float32 and bfloat16 data types in the original graph, some float32 and bfloat16 operators are automatically reduced in precision to float16 according to the built-in optimization strategy, thereby improving system performance and reducing memory usage with minimal precision loss.

    If this mode is configured, you can view the value of the “precision\_reduce” parameter in the built-in optimization strategy file `${INSTALL_DIR}/opp/built-in/op_impl/ai_core/tbe/config/xxx/aic-xxx-ops-info-*.json`:

  - If the value is true (trustlist), the current float32 and bfloat16 operators are allowed to reduce precision to float16.
  - If the value is false (blocklist), the current float32 and bfloat16 operators are not allowed to reduce precision to float16, and the corresponding operators still use float32 or bfloat16 precision.
  - If the operator in the network model does not have this parameter configured (graylist), the mixed precision processing mechanism of the current operator is consistent with that of the previous operator. That is, if the previous operator supports precision reduction, the current operator also supports precision reduction; if the previous operator does not allow precision reduction, the current operator does not support precision reduction either.

- **mixed\_bfloat16:**

    Indicates that mixed precision bfloat16 and float32 data types are used to process the neural network. For operators of the float32 data type in the original graph, some float32 operators are automatically reduced in precision to bfloat16 according to the built-in optimization strategy, thereby improving system performance and reducing memory usage with minimal precision loss. If an operator does not support bfloat16 and float32, an AI CPU operator is used for computation. If the AI CPU operator does not support them either, an error is reported.

    If this mode is configured, you can view the value of the “precision\_reduce” parameter in the `${INSTALL_DIR}/opp/built-in/op_impl/ai_core/tbe/config/xxx/aic-xxx-ops-info-*.json` built-in optimization strategy file:

  - If the value is true (trustlist), the current float32 operators are allowed to reduce precision to bfloat16.
  - If the value is false (blocklist), the current float32 operators are not allowed to reduce precision to bfloat16, and the corresponding operators still use float32 precision.
  - If the operator in the network model does not have this parameter configured (graylist), the mixed precision processing mechanism of the current operator is consistent with that of the previous operator. That is, if the previous operator supports precision reduction, the current operator also supports precision reduction; if the previous operator does not allow precision reduction, the current operator does not support precision reduction either.

- **mixed\_hif8:**

    Enables the automatic mixed precision function, which means that hifloat8 (for details about this data type, see [Link](https://arxiv.org/abs/2409.16626?context=cs.AR)), float16, bfloat16, and float32 data types are used together to process the neural network. For operators of the float16, bfloat16, and float32 data types in the original graph, some float16, bfloat16, and float32 operators are automatically reduced in precision to hifloat8 according to the built-in optimization strategy, thereby improving system performance and reducing memory usage with minimal precision loss.

    If this mode is configured, you can view the value of the “**precision\_reduce**” parameter in the built-in optimization strategy file `${INSTALL_DIR}/opp/built-in/op_impl/ai_core/tbe/config/xxx/aic-xxx-ops-info-*.json`:

  - If the value is true (trustlist), the current float16, bfloat16, and float32 operators are allowed to be reduced in precision to hifloat8.
  - If the value is false (blocklist), the current float16, bfloat16, and float32 operators are not allowed to be reduced in precision to hifloat8, and the corresponding operators still use float16, bfloat16, or float32 precision.
  - If this parameter is not configured for the operators in the original graph (graylist), the mixed precision processing mechanism of the current operator is consistent with that of the previous operator. That is, if the previous operator supports precision reduction, the current operator also supports precision reduction; if the previous operator does not allow precision reduction, the current operator does not support precision reduction either.

- **cube\_hif8:**

    Indicates that when the cube operators in the original graph support both hifloat8 and float16, bfloat16, or float32 data types, the hifloat8 data type is forcibly selected.

In the preceding path, replace $\{INSTALL\_DIR\} with the file storage path after the CANN software is installed. For example, if you install the software as the root user, the default storage path of the installed files is /usr/local/Ascend/cann. Replace xxx based on the actual product.

**Parameter value constraints:**

<!-- @ref: ge/res/docs/zh/user_guides/atc_tools/CLI_options/--precision_mode_v2_res.md#id1 -->

<!-- npu="950,A3,910b,310b" id8 -->
- **The bfloat16 data type is supported only on the following product models**:

    <!-- npu="910b" id1 -->
    Atlas A2 products
    <!-- end id1 -->

    <!-- npu="A3" id2 -->
    Atlas A3 products
    <!-- end id2 -->

    <!-- npu="310b" id3 -->
    Atlas 200I/500 A2 inference product
    <!-- end id3 -->

    <!-- npu="950" id4 -->
    Ascend 950PR&950DT products
    <!-- end id4 -->
<!-- end id8 -->

<!-- npu="950" id5 -->
- **The hif8 data type is supported only on the following product models**:

    Ascend 950PR&950DT products

<!-- end id5 -->
- This parameter prioritizes performance by default, which may cause precision overflow during subsequent inference. If a precision issue occurs during inference, see "Model Inference \> Precision/Performance Optimization \> Suggestions for Improving Model Inference Precision" in *[Application Development](https://gitcode.com/cann/docs/blob/9.2.0/docs/zh/app-dev/c/00_acl_cpp_dev.md)* to locate the issue.
- If you focus on precision issues, you can change it to another value, such as **origin.**

## Recommended Configurations and Benefits

Different precision modes lead to different model precision and performance, as described below:

Precision ranking (from high to low): origin\>mixed\_float16\>fp16\>mixed\_bfloat16\>mixed\_hif8; performance ranking (from good to poor): mixed\_hif8\>mixed\_bfloat16\>fp16\>=mixed\_float16\>origin

## Example

```bash
atc --precision_mode_v2=fp16 ...
```

## Usage Constraints

In mixed precision scenarios, if inference performance degrades after a version upgrade, you are advised to use the AOE tool to perform tuning again. After tuning is complete, load the path of the custom knowledge base generated by operator tuning through the [--op\_bank\_path](--op_bank_path.md) parameter, and then perform model conversion again.

For details about operator tuning, see *[AOE Tuning Tool](https://www.hiascend.com/document/detail/en/CANNCommunityEdition/920/devaids/aoe/auxiliarydevtool_aoe_0001.html)*.
