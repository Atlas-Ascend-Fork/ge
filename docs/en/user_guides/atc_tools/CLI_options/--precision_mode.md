# --precision\_mode

<!-- md-trans-meta sourceCommit=bd3de52c8a808742e9c01b524792105f46cad44c translatedAt=2026-10-22T03:36:35.386Z pushedAt=2026-10-27T12:15:54.097Z -->

## Product Support

Supported on all chips

## Feature Description

Sets the precision mode of the network model.

## Related Parameters

- This parameter cannot be used together with the [--precision\_mode\_v2](--precision_mode_v2.md) parameter. It is recommended to use the --precision\_mode\_v2 parameter, which is newly added in the new version and has clearer option value semantics for easier understanding.
- When the value is **allow\_mix\_precision**, if you want to make adjustments based on the built-in optimization strategy and specify which operators allow precision reduction and which do not, see the [--modify\_mixlist](--modify_mixlist.md) parameter settings.
- In inference scenarios, using the [--precision\_mode](--precision_mode.md) parameter to set the precision mode of the entire network model may cause performance or precision issues for individual operators. In this case, you can use the [--keep\_dtype](--keep_dtype.md) parameter to keep the computation precision of individual operators unchanged when the original network model is compiled. However, when the [--precision\_mode](--precision_mode.md) parameter is set to **must\_keep\_origin\_dtype**, the [--keep\_dtype](--keep_dtype.md) parameter does not take effect.

## Parameter Values

**Parameter value:**

- **force\_fp32/cube\_fp16in\_fp32out:**

    Configuring force\_fp32 or cube\_fp16in\_fp32out has the same effect. This option indicates that when an operator on the AI Core supports both float32 and float16 data types, the system selects different processing methods based on the operator type. cube\_fp16in\_fp32out is newly added in the new version, and its semantics are clearer for matrix computation operators.

  - For matrix computation operators, the system processes them based on the support status of the operator implementation:
        1. Preferentially select float16 as the input data type and float32 as the output data type;
        2. If the scenario in 1 is not supported, select float32 as the input data type and float32 as the output data type;
        3. If the scenario in 2 is not supported, select float16 as the input data type and float16 as the output data type;
        4. If the scenario in 3 is not supported, an error is reported.

  - For vector computation operators, it indicates that the operator precision in the original image is float16 or bfloat16, and float32 is forcibly selected.

      If some operators exist in the original image and the implementation of these operators on the AI Core does not support float32, for example, an operator only supports the float16 type, this parameter does not take effect and the supported float16 is still used. If the implementation of these operators on the AI Core does not support float32 and a blocklist is also configured (precision\_reduce = false), the float32 AI CPU operators are used. If the AI CPU operators are also not supported, an error is reported during execution.

- **force\_fp16 (default):**

    Indicates that when the operator precision in the original image is float16, bfloat16, or float32, float16 is forcibly selected.

- **allow\_fp32\_to\_fp16:**
  - For matrix operators:
    - If the operator precision in the original image is float32, the precision is preferentially reduced to float16. If the operator on the AI Core does not support float16, float32 is selected. If the operator on the AI Core does not support float32, the AI CPU operator is used for computation. If the AI CPU operator is also not supported, an error is reported during execution.
    - If the operator precision in the original image is bfloat16, the original image precision bfloat16 is preferentially used. If the operator on the AI Core does not support bfloat16, float32 is selected. If the operator on the AI Core does not support float32, the precision is directly reduced to float16. If the operator on the AI Core does not support float16, the AI CPU operator is used for computation. If the AI CPU operator is also not supported, an error is reported during execution.

  - For vector operators, the original image precision is preferentially retained:
    - If the operator precision in the original image is float32, the original image precision float32 is preferentially used. If the operator on the AI Core does not support float32, the precision is directly reduced to float16. If the operator on the AI Core does not support float16, the AI CPU operator is used for computation. If the AI CPU operator is also not supported, an error is reported during execution.
    - If the operator precision in the original image is bfloat16, the original image precision bfloat16 is preferentially used. If the operator on the AI Core does not support bfloat16, float32 is selected. If the operator on the AI Core does not support float32, the precision is directly reduced to float16. If the operator on the AI Core does not support float16, the AI CPU operator is used for computation. If the AI CPU operator is also not supported, an error is reported during execution.

- **must\_keep\_origin\_dtype:**

    Keep the original image precision.

  - If the precision of an operator in the original image is float16, and the implementation of this operator on the AI Core does not support float16 but only supports float32 and bfloat16, the system automatically uses the higher precision float32.
  - If the precision of an operator in the original image is float16, and the implementation of this operator on the AI Core does not support float16 but only supports bfloat16, the float16 AI CPU operator is used. If the AI CPU operator is also not supported, an error is reported.
  - If the precision of an operator in the original image is float32, and the implementation of this operator on the AI Core does not support the float32 type but only supports the float16 type, the float32 AI CPU operator is used. If the AI CPU operator is also not supported, an error is reported.

- **allow\_mix\_precision/allow\_mix\_precision\_fp16:**

    Configured as allow\_mix\_precision or allow\_mix\_precision\_fp16, the effect is the same. Both indicate that the mixed precision float16, bfloat16, and float32 data types are used to process the neural network. allow\_mix\_precision\_fp16 is newly added in the new version, with clearer semantics for easier understanding.

    For operators of the float32 and bfloat16 data types in the original model, according to the built-in optimization strategy, some float32 and bfloat16 operators are automatically reduced to float16 precision, thereby improving system performance and reducing memory usage with minimal precision loss.

    If this mode is configured, you can view the value of the “precision\_reduce” parameter in the built-in optimization strategy file `${INSTALL_DIR}/opp/built-in/op_impl/ai_core/tbe/config/xxx/aic-xxx-ops-info-*.json`:

  - If the value is true (trustlist), it indicates that the current float32 and bfloat16 operators are allowed to be reduced to float16 precision.
  - If the value is false (blocklist), the current float32 and bfloat16 operators are not allowed to reduce precision to float16, and the corresponding operators still use float32 or bfloat16 precision.
  - If the operator in the network model is not configured with this parameter (graylist), the mixed precision processing mechanism of the current operator remains consistent with that of the previous operator. That is, if the previous operator supports precision reduction, the current operator also supports precision reduction; if the previous operator does not allow precision reduction, the current operator does not support precision reduction either.

- **allow\_mix\_precision\_bf16:**

    Indicates the process of using mixed precision bfloat16 and float32 data types to process a neural network. For operators of the float32 data type in the original model, some float32 operators are automatically reduced to bfloat16 according to the built-in optimization strategy, thereby improving system performance and reducing memory usage with minimal precision loss. If the operator on the AI Core does not support bfloat16 and float32, the AI CPU operator is used for computation; if the AI CPU operator does not support them either, an error is reported.

    If this mode is configured, you can view the value of the “precision\_reduce” parameter in the built-in optimization strategy file `${INSTALL_DIR}/opp/built-in/op_impl/ai_core/tbe/config/xxx/aic-xxx-ops-info-*.json`:

  - If the value is true (trustlist), the current float32 operator is allowed to reduce precision to bfloat16.
  - If the value is false (blocklist), the current float32 operator is not allowed to reduce precision to bfloat16, and the corresponding operator still uses float32 precision.
  - If the operator in the network model is not configured with this parameter (graylist), the mixed precision processing mechanism of the current operator remains consistent with that of the previous operator. That is, if the previous operator supports precision reduction, the current operator also supports precision reduction; if the previous operator does not allow precision reduction, the current operator does not support precision reduction either.

- **allow\_fp32\_to\_bf16:**
  - If the operator precision in the original image is float32, the original image precision float32 is used preferentially. If the operator on the AI Core does not support float32, the precision is reduced to bfloat16; if the operator on the AI Core does not support bfloat16, the AI CPU operator is used for computation; if the AI CPU operator does not support it either, an error is reported.
  - If the operator precision in the original image is bfloat16, the original image precision bfloat16 is used first. If the operator on the AI Core does not support bfloat16, float32 is selected. If the operator on the AI Core does not support float32, the AI CPU operator is used for computation. If the AI CPU operator does not support it either, an error is reported during execution.

Replace $\{INSTALL\_DIR\} in the preceding path with the file storage path after CANN software installation. For example, when installed by the root user, the default file storage path is /usr/local/Ascend/cann. Select xxx based on the actual product.

**Parameter value constraints:**

<!-- @ref: ge/res/docs/zh/user_guides/atc_tools/CLI_options/--precision_mode_res.md#id1 -->

<!-- npu="950,A3,910b,310b" id6 -->
- **The bfloat16 data type is only supported on the following product types**:

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
<!-- end id6 -->

- This parameter defaults to performance first, which may cause precision overflow issues during subsequent inference. If a precision issue occurs during inference, see "Model Inference \> Precision/Performance Optimization \> Suggestions for Improving Model Inference Precision" in *[Application Development](https://gitcode.com/cann/docs/blob/9.2.0/docs/zh/app-dev/c/00_acl_cpp_dev.md)* to locate the issue.
- If you focus on precision issues, you can change it to other values, such as **must\_keep\_origin\_dtype.**

## Recommended Configuration and Benefits

Different precision modes result in different network model precision and performance, as follows:

Precision ranking (from high to low): force\_fp32\>must\_keep\_origin\_dtype\>allow\_fp32\_to\_fp16\>allow\_mix\_precision\>force\_fp16

Performance ranking (from good to poor): force\_fp16\>=allow\_mix\_precision\>allow\_fp32\_to\_fp16\>must\_keep\_origin\_dtype\>force\_fp32

## Example

```bash
atc --precision_mode=force_fp16 ...
```

## Constraints

In mixed precision scenarios, if inference performance degrades after a version upgrade, you are advised to use the AOE tool to perform tuning again. After tuning is complete, load the path of the custom knowledge base generated by operator tuning through the [--op\_bank\_path](--op_bank_path.md) parameter, and then perform model conversion again.

For details about operator tuning, see [*AOE Tuning Tool*](https://www.hiascend.com/document/detail/en/CANNCommunityEdition/920/devaids/aoe/auxiliarydevtool_aoe_0001.html).
