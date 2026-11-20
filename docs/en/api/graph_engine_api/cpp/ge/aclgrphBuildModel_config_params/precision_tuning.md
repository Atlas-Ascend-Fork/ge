# Precision Tuning

<!-- npu="950,A3,910b,910,310p,310b" id1 -->
<!-- md-trans-meta sourceCommit=a0ebfd022a9200d2cd068bee8ba395fc30eab4d9 translatedAt=2026-11-17T02:56:34.750Z pushedAt=2026-11-18T02:41:08.909Z nl=2 -->

## ALLOW\_HF32

Whether to enable the function of automatically replacing the float32 data type with HF32. In the current version, this parameter takes effect only for Conv-type operators and Matmul-type operators. The actual corresponding options parameter of this parameter is `ge.exec.allow_hf32`.

HF32 is a single-precision floating-point type introduced by Ascend specifically for internal operator computation. A comparison with other common data types is shown in the following figure. As shown, HF32 and float32 support the same value range, but the mantissa precision (11 bits) of HF32 is close to that of FP16 (10 bits). By reducing precision, the HF32 single-precision data type replaces the original float32 single-precision data type, which greatly reduces the space occupied by data and improves performance.

![Illustration](../../../figures/hf32.png)

**Parameter values:**

- true: For Conv-type operators and Matmul-type operators, enable the function of automatically converting the FP32 data type to the HF32 data type.

    For the specific operators involved, see opp/built-in/op\_impl/ai\_core/tbe/impl\_mode/allow\_hf32\_matmul\_**t**\_conv\_**t**.ini in the file storage path after CANN software installation. This file does not support user modification.

- false: For Conv-type operators and Matmul-type operators, disable the function of automatically converting the FP32 data type to the HF32 data type.

    For the specific operators involved, see opp/built-in/op\_impl/ai\_core/tbe/impl\_mode/allow\_hf32\_matmul\_**f**\_conv\_**f**.ini in the file storage path after CANN software installation. This file does not support user modification.

**Default value**: For Conv-type operators, the conversion from FP32 to HF32 is enabled by default; for Matmul-type operators, this conversion function is disabled by default.

**Usage constraints:**

- For the same operator, if enable\_hi\_float\_32\_execution or enable\_float\_32\_execution is configured through the OP\_PRECISION\_MODE parameter, it is not recommended to use it simultaneously with the ALLOW\_HF32 parameter in this scenario. If they are used simultaneously, the priority is as follows:

    OP\_PRECISION\_MODE\(ByNodeName, set the precision mode by node name \) \> ALLOW\_HF32 \> OP\_PRECISION\_MODE\(ByOpType, set the precision mode by operator type \).

- Because ALLOW\_HF32 automatically replaces float32 with HF32, for this parameter to take effect, the input or output type of the involved operators must be float32. Because the default value of the PRECISION\_MODE\_V2 parameter is fp16, operators in the original network model whose type is float32 are forcibly converted to float16. In this scenario, the ALLOW\_HF32 parameter does not take effect. It is recommended to modify the PRECISION\_MODE\_V2 parameter value to **origin** (the default value of the PRECISION\_MODE parameter is force\_fp16; it is recommended to modify it to **must\_keep\_origin\_dtype** or **force\_fp32**).

**Configuration example:**

```c++
{ge::ir_option::ALLOW_HF32, "true"}
```

**Applicable products:**

<!-- npu="950" id21 -->
- Ascend 950PR&950DT products: Supported
<!-- end id21 -->
<!-- npu="A3" id22 -->
- Atlas A3 products: Supported
<!-- end id22 -->
<!-- npu="910b" id23 -->
- Atlas A2 products: Supported
<!-- end id23 -->
<!-- npu="310b" id24 -->
- Atlas 200I/500 A2 inference products: Not supported
<!-- end id24 -->
<!-- npu="310p" id25 -->
- Atlas inference products: Not supported
<!-- end id25 -->
<!-- npu="910" id26 -->
- Atlas training products: Not supported
<!-- end id26 -->

<!-- end id1 -->

## PRECISION\_MODE

Sets the precision mode of operators. This parameter cannot be used together with **PRECISION\_MODE\_V2** in the same graph. It is recommended to use the **PRECISION\_MODE\_V2** parameter. The **options** parameter actually corresponding to this parameter is `ge.exec.precision_mode`.

**Parameter values:**

- **force\_fp32/cube\_fp16in\_fp32out**:

    Configuring force\_fp32 or cube\_fp16in\_fp32out has the same effect. This option indicates that when an operator on the AI Core supports both float32 and float16 data types, the system internally selects different processing methods based on the operator type. cube\_fp16in\_fp32out is newly added in the new version, with clearer semantics for cube computation operators.

  - For cube computation operators, the system internally processes them based on the support status of the operator implementation:

      1. Preferentially select the input data type float16 and the output data type float32;

      2. If the scenario in 1 is not supported, select the input data type float32 and the output data type float32;

      3. If the scenario in 2 is not supported, select the input data type float16 and the output data type float16;

      4. If the scenario in 3 is not supported, an error is reported.

  - For vector computation operators, this indicates that the operator precision in the original graph is float16 or bfloat16, and float32 is forcibly selected.

      If the original graph contains some operators whose implementations on the AI Core do not support float32, for example, an operator that supports only the float16 type, this parameter does not take effect and the supported float16 is still used. If the implementation of the operator on the AI Core does not support float32 and a blocklist is also configured (precision\_reduce = false), the float32 AI CPU operator is used. If the AI CPU operator does not support it either, an execution error is reported.

- **force\_fp16 (default value)**:

    This indicates that when the operator precision in the original graph is float16, bfloat16, or float32, float16 is forcibly selected.

- **allow\_fp32\_to\_fp16**:
  - For cube operators:
    - If the operator precision in the original graph is float32, the precision is preferentially reduced to float16. If the operator on the AI Core does not support float16, float32 is selected. If the operator on the AI Core does not support float32, the AI CPU operator is used for computation. If the AI CPU operator does not support it either, an execution error is reported.
    - If the operator precision in the original graph is bfloat16, the original precision bfloat16 is preferentially used. If the operator on the AI Core does not support bfloat16, float32 is selected. If the operator on the AI Core does not support float32, the precision is directly reduced to float16. If the operator on the AI Core does not support float16, the AI CPU operator is used for computation. If the AI CPU operator does not support it either, an execution error is reported.

  - For vector computation operators, the original precision is preferentially retained:
    - If the operator precision in the original graph is float32, the original precision float32 is preferentially used. If the operator on the AI Core does not support float32, the precision is directly reduced to float16. If the operator on the AI Core does not support float16, the AI CPU operator is used for computation. If the AI CPU operator does not support it either, an execution error is reported.
    - If the precision of an operator in the original graph is bfloat16, the original precision bfloat16 is used first. If the operator does not support bfloat16 on the AI Core, float32 is selected. If the operator does not support float32 on the AI Core, the precision is directly reduced to float16. If the operator does not support float16 on the AI Core, the AI CPU operator is used for computation. If the AI CPU operator does not support it either, an execution error is reported.

- **must\_keep\_origin\_dtype**:

    The original precision is retained.

  - If the precision of an operator in the original graph is float16, and the implementation of this operator on the AI Core does not support float16 but supports only float32 and bfloat16, the system automatically adopts the high precision float32.
  - If the precision of an operator in the original graph is float16, and the implementation of this operator on the AI Core does not support float16 but supports only bfloat16, the float16 AI CPU operator is used. If the AI CPU operator does not support it either, an execution error is reported.
  - If the precision of an operator in the original graph is float32, and the implementation of this operator on the AI Core does not support float32 but supports only float16, the float32 AI CPU operator is used. If the AI CPU operator does not support it either, an execution error is reported.

- **allow\_mix\_precision/allow\_mix\_precision\_fp16**:

    When configured as allow_mix_precision or allow_mix_precision_fp16, the effect is the same. Both indicate the process of using mixed precision float16, bfloat16, and float32 data types to process the neural network. allow_mix_precision_fp16 is newly added in the new version and has clearer semantics for easier understanding.

    For operators with float32 and bfloat16 data types in the original model, according to the built-in optimization strategy, some float32 and bfloat16 operators are automatically reduced to float16, thereby improving system performance and reducing memory usage with minimal precision loss.

    If this mode is configured, you can view the value of the "precision\_reduce" parameter in the built-in optimization strategy file `${INSTALL_DIR}/opp/built-in/op_impl/ai_core/tbe/config/xxx/aic-xxx-ops-info-*.json`:

  - If the value is **true** (allowlist), operators of the current float32 and bfloat16 types are allowed to be reduced to float16 precision.
  - If the value is **false** (blocklist), operators of the current float32 and bfloat16 types are not allowed to be reduced to float16 precision, and the corresponding operators still use float32 or bfloat16 precision.
  - If the operator in the network model does not have this parameter configured (graylist), the mixed precision processing mechanism of the current operator remains consistent with that of the previous operator. That is, if the previous operator supports precision reduction, the current operator also supports precision reduction; if the previous operator does not allow precision reduction, the current operator does not support precision reduction either.

- **allow\_mix\_precision\_bf16:**

    Indicates the process of using mixed precision bfloat16 and float32 data types to process the neural network. For operators of the float32 data type in the original model, some float32 operators are automatically reduced to bfloat16 precision according to the built-in optimization strategy, thereby improving system performance and reducing memory usage with minimal precision loss. If the operator on the AI Core does not support bfloat16 and float32, the AI CPU operator is used for computation; if the AI CPU operator does not support them either, an execution error is reported.

    If this mode is configured, you can view the value of the "precision\_reduce" parameter in the built-in optimization strategy file `${INSTALL_DIR}/opp/built-in/op_impl/ai_core/tbe/config/xxx/aic-xxx-ops-info-*.json`:

  - If the value is **true** (allowlist), operators of the current float32 type are allowed to be reduced to bfloat16 precision.
  - If the value is **false** (blocklist), operators of the current float32 type are not allowed to be reduced to bfloat16 precision, and the corresponding operators still use float32 precision.
  - If the operator in the network model does not have this parameter configured (graylist), the mixed precision processing mechanism of the current operator remains consistent with that of the previous operator. That is, if the previous operator supports precision reduction, the current operator also supports precision reduction; if the previous operator does not allow precision reduction, the current operator does not support precision reduction either.

- **allow\_fp32\_to\_bf16:**
  - If the operator precision in the original graph is float32, the original precision float32 is used first. If the operator on the AI Core does not support float32, the precision is reduced to bfloat16. If the operator on the AI Core does not support bfloat16, the AI CPU operator is used for computation. If the AI CPU operator does not support it either, an execution error is reported.
  - If the operator precision in the original graph is bfloat16, the original precision bfloat16 is used first. If the operator on the AI Core does not support bfloat16, float32 is selected. If the operator on the AI Core does not support float32, the AI CPU operator is used for computation. If the AI CPU operator does not support it either, an execution error is reported.

In the preceding path, replace **$\{INSTALL\_DIR\}** with the file storage path after CANN software installation. Taking installation by the root user as an example, the default file storage path after installation is **/usr/local/Ascend/cann**.

**Parameter value constraints:**

<!-- @ref: ge/res/docs/en/api/graph_engine_api/cpp/ge/aclgrphBuildModel_config_params/precision_tuning_res.md#id1 -->
<!-- npu="IPV350" id4 -->
- **IPV350 does not support the bfloat16 type, and the corresponding option is not supported either.**
<!-- end id4 -->
<!-- npu="950,A3,910b,310b" id3 -->
- **The bfloat16 data type is supported only on the following product types**:

    Atlas A2 series products

    Atlas A3 series products

    Atlas 200I/500 A2 inference products

    Ascend 950PR&950DT series products
<!-- end id3 -->

- This parameter defaults to performance priority, which may cause precision overflow during subsequent inference. If a precision issue occurs during inference, see "Model inference \> Precision/Performance optimization \> Suggestions for improving model inference precision" in [*Application Development*](https://gitcode.com/cann/docs/blob/master/docs/zh/app-dev/c/00_acl_cpp_dev.md) for troubleshooting.
- If users focus on precision issues, they can change it to other values, such as **must\_keep\_origin\_dtype.**

**Configuration example:**

```c++
{ge::ir_option::PRECISION_MODE, "force_fp16"}
```

**Applicable products:**

Supported on all chips.

## PRECISION\_MODE\_V2

Sets the precision mode of operators. This parameter cannot be used together with **PRECISION\_MODE** in the same graph. It is recommended to use the **PRECISION\_MODE\_V2** parameter. The **options** parameter actually corresponding to this parameter is `ge.exec.precision_mode_v2`.

**Parameter values:**

- **fp16 (default value):**

    Indicates that when the operator precision in the original graph is float16, bfloat16, or float32, float16 is forcibly selected.

- **origin:**

    Keeps the original precision of the graph.

  - If the precision of an operator in the original graph is float16, and the implementation of this operator on the AI Core does not support float16 but only supports float32 and bfloat16, then the system automatically uses the high precision float32.
  - If the precision of an operator in the original graph is float16, and the implementation of this operator on the AI Core does not support float16 but only supports bfloat16, then the float16 AI CPU operator is used; if the AI CPU operator is also not supported, an execution error is reported.
  - If the precision of an operator in the original graph is float32, and the implementation of this operator on the AI Core does not support the float32 type but only supports the float16 type, then the float32 AI CPU operator is used; if the AI CPU operator is also not supported, an execution error is reported.

- **cube\_fp16in\_fp32out**:

    When the operator in the AI Core supports both float32 and float16 data types, the system internally selects different processing methods based on the operator type.

  - For cube computation operators, the system internally processes them based on the operator implementation support:

      1. Preferentially select the input data type as float16 and the output data type as float32;

      2. If the scenario in 1 is not supported, select the input data type as float32 and the output data type as float32;

      3. If the scenario in 2 is not supported, select the input data type as float16 and the output data type as float16;

      4. If the scenario in 3 is not supported, an error is reported.

  - For vector computation operators, it indicates that the operator precision in the original graph is float16 or bfloat16, and float32 is forcibly selected.

      If the original graph contains some operators whose implementation in the AI Core does not support float32, for example, an operator supports only float16, this parameter does not take effect and the supported float16 is still used; if the operator implementation in the AI Core does not support float32 and a blocklist is also configured (precision\_reduce = false), the float32 AI CPU operator is used; if the AI CPU operator is also not supported, an execution error is reported.

- **mixed\_float16**:

    Indicates that mixed precision float16, bfloat16, and float32 data types are used to process the neural network. For operators of float32 and bfloat16 data types in the original graph, some float32 and bfloat16 operators are automatically reduced to float16 precision according to the built-in optimization strategy, thereby improving system performance and reducing memory usage with minimal precision loss.

    If this mode is configured, you can view the value of the "precision\_reduce" parameter in the `${INSTALL_DIR}/opp/built-in/op_impl/ai_core/tbe/config/xxx/aic-xxx-ops-info-*.json` built-in optimization strategy file:

  - If the value is **true** (allowlist), the current float32 and bfloat16 operators are allowed to be reduced to float16 precision.
  - If the value is **false** (blocklist), the current float32 and bfloat16 operators are not allowed to be reduced to float16 precision, and the corresponding operators still use float32 or bfloat16 precision.
  - If the operator in the network model does not have this parameter configured (graylist), the mixed precision processing mechanism of the current operator remains consistent with that of the previous operator. That is, if the previous operator supports precision reduction, the current operator also supports precision reduction; if the previous operator does not allow precision reduction, the current operator does not support precision reduction either.

- **mixed\_bfloat16**:

    Indicates that mixed precision bfloat16 and float32 data types are used to process the neural network. For operators of float32 data type in the original graph, some float32 operators are automatically reduced to bfloat16 precision according to the built-in optimization strategy, thereby improving system performance and reducing memory usage with minimal precision loss. If an operator does not support bfloat16 and float32, the AI CPU operator is used for computation; if the AI CPU operator also does not support them, an execution error is reported.

    If this mode is configured, you can view the value of the "precision\_reduce" parameter in the `${INSTALL_DIR}/opp/built-in/op_impl/ai_core/tbe/config/xxx/aic-xxx-ops-info-*.json` built-in optimization strategy file:

  - If the value is **true** (allowlist), the current float32 operators are allowed to be reduced to bfloat16 precision.
  - If the value is **false** (blocklist), the current float32 operators are not allowed to be reduced to bfloat16 precision, and the corresponding operators still use float32 precision.
  - If the operator in the network model is not configured with this parameter (graylist), the mixed precision processing mechanism of the current operator remains consistent with that of the previous operator. That is, if the previous operator supports precision reduction, the current operator also supports precision reduction; if the previous operator does not allow precision reduction, the current operator does not support precision reduction either.

- **mixed\_hif8**:

    Enables the automatic mixed precision function, which means that hifloat8 (for an introduction to this data type, see [Link](https://arxiv.org/abs/2409.16626?context=cs.AR)), float16, bfloat16, and float32 data types are used together to process the neural network. For operators of the float16, bfloat16, and float32 data types in the original graph, some float16, bfloat16, and float32 operators are automatically reduced in precision to hifloat8 according to the built-in optimization strategy, thereby improving system performance and reducing memory usage with minimal precision loss.

    If this mode is configured, you can view the value of the "**precision\_reduce**" parameter in the built-in optimization strategy file `${INSTALL_DIR}/opp/built-in/op_impl/ai_core/tbe/config/xxx/aic-xxx-ops-info-*.json`:

  - If the value is **true** (allowlist), it indicates that the current float16, bfloat16, and float32 operators are allowed to be reduced in precision to hifloat8.
  - If the value is **false** (blocklist), the current float16, bfloat16, and float32 operators are not allowed to be reduced in precision to hifloat8, and the corresponding operators still use float16, bfloat16, or float32 precision.
  - If the operator in the original graph is not configured with this parameter (graylist), the mixed precision processing mechanism of the current operator remains consistent with that of the previous operator. That is, if the previous operator supports precision reduction, the current operator also supports precision reduction; if the previous operator does not allow precision reduction, the current operator does not support precision reduction either.

- **cube\_hif8**:

   Indicates that when a cube operator in the original graph supports both hifloat8 and float16, bfloat16, or float32 data types, the hifloat8 data type is forcibly selected.

Replace **$\{INSTALL\_DIR\}** in the preceding path with the file storage path after CANN software installation. Taking installation by the root user as an example, the default file storage path after installation is **/usr/local/Ascend/cann**.

**Parameter value constraints:**

<!-- @ref: ge/res/docs/en/api/graph_engine_api/cpp/ge/aclgrphBuildModel_config_params/precision_tuning_res.md#id2 -->
<!-- npu="IPV350" id14 -->
- **IPV350 does not support the bfloat16 and hif8 types, and the corresponding options are not supported either.**
<!-- end id14 -->
<!-- npu="950,A3,910b,310b" id9 -->
- **The bfloat16 data type is supported only on the following product models**:

    <!-- npu="950" id10 -->
    Ascend 950PR&950DT series products
    <!-- end id10 -->

    <!-- npu="A3" id11 -->
    Atlas A3 series products
    <!-- end id11 -->

    <!-- npu="910b" id12 -->
    Atlas A2 series products
    <!-- end id12 -->

    <!-- npu="310b" id13 -->
    Atlas 200I/500 A2 inference products
    <!-- end id13 -->

<!-- end id9 -->
<!-- npu="950" id8 -->
- **The hif8 data type is supported only on the following product models**:

  Ascend 950PR&950DT series products
<!-- end id8 -->

- This parameter defaults to performance-first, which may cause precision overflow issues during subsequent inference. If precision issues occur during inference, see "Model inference \> Precision/Performance optimization \> Model inference precision improvement suggestions" in [*Application Development*](https://gitcode.com/cann/docs/blob/master/docs/zh/app-dev/c/00_acl_cpp_dev.md) for troubleshooting.
- If users focus on precision issues, they can change it to other values, such as **origin.**

**Configuration example:**

```c++
{ge::ir_option::PRECISION_MODE_V2, "fp16"}
```

**Applicable products:**

Supported on all chips.

## MODIFY\_MIXLIST

In mixed precision scenarios, this parameter specifies the path and file name of the mixed precision allowlist/blocklist/graylist, allowing you to specify which operators are allowed to reduce precision and which are not. The actual **options** parameter corresponding to this parameter is `ge.exec.modify_mixlist`.

**Parameter values:**

Configure it as a path and file name. The file is in JSON format. The allowlist/blocklist/graylist can be viewed from the flag parameter value under the "precision\_reduce" parameter in the built-in optimization strategy file `${INSTALL_DIR}/opp/built-in/op_impl/ai_core/tbe/config/xxx/aic-xxx-ops-info-*.json` (where **$\{INSTALL\_DIR\}** should be replaced with the file storage path after CANN software installation. Taking root user installation as an example, the default storage path after installation is **/usr/local/Ascend/cann**. Replace **xxx** with the actual product.):

- If the value is **true** (allowlist), it indicates that precision reduction is **allowed** in mixed precision mode.
- If the value is **false** (blocklist), it indicates that precision reduction is **not allowed** in mixed precision mode.
- If this parameter is not configured (graylist), it indicates that in mixed precision mode, the mixed precision processing mechanism of the current operator remains consistent with that of the previous operator. That is, if the previous operator supports precision reduction, the current operator also supports it; if the previous operator does not allow precision reduction, the current operator does not support it either.

How to enable mixed precision:

- Set the **PRECISION\_MODE** parameter to **allow\_mix\_precision**, **allow\_mix\_precision\_fp16**, or **allow\_mix\_precision\_bf16**.
- Set the **PRECISION\_MODE\_V2** parameter to **mixed\_float16**, **mixed\_bfloat16**, or **mixed\_hif8**. It cannot be configured together with the **PRECISION\_MODE** parameter. It is recommended to use **PRECISION\_MODE\_V2**.

    <!-- npu="950,A3,910b,310b" id15 -->
    bf16 is supported only on the following product models:

    <!-- npu="950" id16 -->
    Ascend 950PR&950DT series products
    <!-- end id16 -->

    <!-- npu="A3" id17 -->
    Atlas A3 series products
    <!-- end id17 -->

    <!-- npu="910b" id18 -->
    Atlas A2 series products
    <!-- end id18 -->

    <!-- npu="310b" id19 -->
    Atlas 200I/500 A2 inference products
    <!-- end id19 -->
    <!-- end id15 -->

    <!-- npu="950" id20 -->
    hif8 is supported only on Ascend 950PR&950DT series products.
    <!-- end id20 -->
    <!-- @ref: ge/res/docs/en/api/graph_engine_api/cpp/ge/aclgrphBuildModel_config_params/precision_tuning_res.md#id3 -->

**Configuration example:**

```c++
{ge::ir_option::MODIFY_MIXLIST, "/home/test/ops_info.json"}
```

**ops\_info.json** allows you to specify operator types, with multiple operators separated by English commas. An example is as follows:

```json
{
  "black-list": {                  // Blocklist.
     "to-remove": [                // Convert blocklist operators to graylist operators. When configuring this parameter, ensure that the operators to be converted already exist in the blocklist.
     "Xlog1py"
     ],
     "to-add": [                   // Convert allowlist or graylist operators to blocklist operators.
     "Matmul",
     "Cast"
     ]
  },
  "white-list": {                  // Allowlist.
     "to-remove": [                // Convert allowlist operators to graylist operators. When configuring this parameter, ensure that the operators to be converted already exist in the allowlist.
     "Conv2D"
     ],
     "to-add": [                   // Convert blocklist or graylist operators to allowlist operators.
     "Bias"
     ]
  }
}
```

The operators shown in the preceding configuration file sample are for reference only. Configure them based on the actual hardware environment and the specific built-in optimization strategies of the operators. A sample query for the allowlist, blocklist, and graylist is as follows:

```json
"Conv2D":{
    "precision_reduce":{
        "flag":"true"
     }
},
```

**true**: allowlist. **false**: blocklist. Not configured: graylist.

**Applicable products:**

Supported on all chips.

## CUSTOMIZE\_DTYPES

The actual **options** parameter corresponding to this parameter is `ge.customizeDtypes`.

This parameter customizes the computation precision of operators during model compilation. Other operators in the model are compiled with the precision mode specified by **PRECISION\_MODE** or **PRECISION\_MODE\_V2**. This parameter must be configured as the path and file name of a configuration file, for example: **/home/test/customize\_dtypes.cfg**.

**Parameter value constraints:**

- The configuration file lists the operator names or operator types whose computation precision needs to be customized, with each operator on a separate line. The operator type must be the type of an operator defined based on IR.
- For the same operator, if both the operator name and operator type are configured, the operator name takes precedence during compilation.
- When specifying the computation precision of an operator, if the operator is fused during compilation, the specified computation precision does not take effect.

Configuration file format requirements:

```text
# Configure by operator name.
Opname1::InputDtype:dtype1,dtype2,…OutputDtype:dtype1,…
Opname2::InputDtype:dtype1,dtype2,…OutputDtype:dtype1,…
# Configure by operator type.
OpType::TypeName1:InputDtype:dtype1,dtype2,…OutputDtype:dtype1,…
OpType::TypeName2:InputDtype:dtype1,dtype2,…OutputDtype:dtype1,…
```

Configuration file example:

```text
# Configure by operator name.
resnet_v1_50/block1/unit_3/bottleneck_v1/Relu::InputDtype:float16,int8,OutputDtype:float16,int8
# Configure by operator type.
OpType::Relu:InputDtype:float16,int8,OutputDtype:float16,int8
```

> [!NOTE]
>
>- The computation precisions supported by an operator can be viewed in the operator information library. The default storage path is `${INSTALL_DIR}/opp/built-in/op_impl/ai_core/tbe/config/xxx/aic-xxx-ops-info-*.json`.
>    Replace **${INSTALL_DIR}** with the file storage path after CANN software installation. For example, if you install the software as the root user, the default file storage path after installation is **/usr/local/Ascend/cann**. Select xxx based on the actual product.
>- The priority specified by this parameter is high, which may cause precision/performance degradation. If the specified dtype is not supported, compilation fails.

**Configuration example:**

```c++
{ge::ir_option::CUSTOMIZE_DTYPES, "/home/test/customize_dtypes.cfg"}
```

**Applicable products:**

Supported on all chips.
