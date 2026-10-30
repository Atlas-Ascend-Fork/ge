# Precision Tuning

<!-- npu="950,A3,910b,910,310p,310b" id14 -->
<!-- md-trans-meta sourceCommit=607806abd035eaf2ab464a02634ae7b347df3984 translatedAt=2026-10-29T01:00:03.751Z nl=2 -->

## ALLOW\_HF32

Whether to enable the automatic replacement of the float32 data type with HF32. In the current version, this parameter takes effect only for Conv operators and Matmul operators. The options parameter that this parameter actually corresponds to is `ge.exec.allow_hf32`.

HF32 is a single-precision floating-point type launched by Ascend specifically for internal operator computation. A comparison with other common data types is shown in the following figure. As shown, HF32 supports the same value range as float32, but its mantissa precision (11 bits) is close to that of FP16 (10 bits). By reducing precision so that the HF32 single-precision data type replaces the original float32 single-precision data type, the space occupied by data can be greatly reduced, thereby improving performance.

![Illustration](../../../figures/hf32.png)

**Parameter values:**

- true: For Conv operators and Matmul operators, enables the automatic conversion from the FP32 data type to the HF32 data type.

    For the specific operators involved, see opp/built-in/op\_impl/ai\_core/tbe/impl\_mode/allow\_hf32\_matmul\_**t**\_conv\_**t**.ini in the file storage path after CANN software installation. This file does not support user modification.

- false: For Conv operators and Matmul operators, disables the automatic conversion from the FP32 data type to the HF32 data type.

    For the specific operators involved, see opp/built-in/op\_impl/ai\_core/tbe/impl\_mode/allow\_hf32\_matmul\_**f**\_conv\_**f**.ini in the file storage path after CANN software installation. This file does not support user modification.

**Default parameter value**: For Conv operators, the conversion from FP32 to HF32 is enabled by default. For Matmul operators, this conversion is disabled by default.

**Usage constraints:**

- For the same operator, if enable\_hi\_float\_32\_execution or enable\_float\_32\_execution is configured through the OP\_PRECISION\_MODE parameter, it is not recommended to use it together with the ALLOW\_HF32 parameter in this scenario. If they are used together, the priority is as follows:

    OP\_PRECISION\_MODE\(ByNodeName, set the precision mode by node name\) \> ALLOW\_HF32 \> OP\_PRECISION\_MODE\(ByOpType, set the precision mode by operator type\).

- Because ALLOW\_HF32 uses HF32 to automatically replace float32, for this parameter to take effect, the input or output type of the involved operators must be float32. Because the default value of the PRECISION\_MODE\_V2 parameter is fp16, operators of type float32 in the original network model are forcibly converted to float16. In this scenario, using the ALLOW\_HF32 parameter does not take effect. It is recommended to change the PRECISION\_MODE\_V2 parameter value to **origin** (the default value of the PRECISION\_MODE parameter is force\_fp16; it is recommended to change it to **must\_keep\_origin\_dtype** or **force\_fp32**).

**Configuration example:**

```c++
{ge::ir_option::ALLOW_HF32, "true"}
```

**Applicable products:**

<!-- npu="950" id2 -->
- Ascend 950PR&950DT products: Supported
<!-- end id2 -->
<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id4 -->
- Atlas A2 products: Supported
<!-- end id4 -->
<!-- npu="310b" id5 -->
- Atlas 200I/500 A2 inference products: Not supported
<!-- end id5 -->
<!-- npu="310p" id6 -->
- Atlas inference products: Not supported
<!-- end id6 -->
<!-- npu="910" id7 -->
- Atlas training products: Not supported
<!-- end id7 -->
<!-- end id14 -->

## PRECISION\_MODE

Sets the precision mode of operators. It cannot be used together with PRECISION\_MODE\_V2 in the same graph. It is recommended to use the PRECISION\_MODE\_V2 parameter instead. The options parameter actually corresponding to this parameter is `ge.exec.precision_mode`.

**Parameter values:**

- **force\_fp32/cube\_fp16in\_fp32out:**

    When configured as force\_fp32 or cube\_fp16in\_fp32out, the effect is the same. This option indicates that when the operator on the AI Core supports both float32 and float16 data types, the system internally selects different processing methods based on the operator type. cube\_fp16in\_fp32out is newly added in the new version. For cube computation operators, this option has clearer semantics.

  - For cube computation-type operators, the system internally processes them based on the support status of the operator implementation:

      1. Preferentially select the input data type as float16 and the output data type as float32;

      2. If the scenario in 1 is not supported, select the input data type as float32 and the output data type as float32;

      3. If the scenario in 2 is not supported, select the input data type as float16 and the output data type as float16;

      4. If the scenario in 3 is not supported, an error is reported.

  - For vector computation-type operators, indicates that when the operator precision in the original graph is float16 or bfloat16, float32 is forcibly selected.

      If the original graph contains some operators whose implementation on the AI Core does not support float32, for example, an operator that supports only float16, this parameter does not take effect and the supported float16 is still used. If the implementation of the operator on the AI Core does not support float32 and a blocklist is also configured (precision\_reduce = false), the float32 AI CPU operator is used. If the AI CPU operator is also unsupported, an error is reported during execution.

- **force\_fp16 (default value):**

    Indicates that when the operator precision in the original graph is float16, bfloat16, or float32, float16 is forcibly selected.

- **allow\_fp32\_to\_fp16:**
  - For cube-type operators:
    - If the operator precision in the original graph is float32, precision is preferentially reduced to float16. If the operator on the AI Core does not support float16, float32 is selected instead. If the operator on the AI Core does not support float32, the AI CPU operator is used for computation. If the AI CPU operator is also unsupported, an error is reported.
    - If the operator precision in the original graph is bfloat16, the original precision bfloat16 is preferentially used. If the operator on the AI Core does not support bfloat16, float32 is selected. If the operator on the AI Core does not support float32, precision is directly reduced to float16. If the operator on the AI Core does not support float16, the AI CPU operator is used for computation. If the AI CPU operator is also unsupported, an error is reported.

  - For vector-type operators, the original precision is preferentially retained:
    - If the operator precision in the original graph is float32, the original precision float32 is preferentially used. If the operator on the AI Core does not support float32, precision is directly reduced to float16. If the operator on the AI Core does not support float16, the AI CPU operator is used for computation. If the AI CPU operator is also unsupported, an error is reported.
    - If the operator precision in the original graph is bfloat16, the original precision bfloat16 is preferentially used. If the operator on the AI Core does not support bfloat16, float32 is selected. If the operator on the AI Core does not support float32, precision is directly reduced to float16. If the operator on the AI Core does not support float16, the AI CPU operator is used for computation. If the AI CPU operator is also unsupported, an error is reported.

- **must\_keep\_origin\_dtype:**

    The original graph precision is maintained.

  - If the precision of an operator in the original graph is float16, and the implementation of the operator in the AI Core does not support float16 but supports only float32 and bfloat16, the system automatically uses the higher precision float32.
  - If the precision of an operator in the original graph is float16, and the implementation of the operator in the AI Core does not support float16 but supports only bfloat16, the float16 AI CPU operator is used. If the AI CPU operator does not support it either, an error is reported.
  - If the precision of an operator in the original graph is float32, and the implementation of the operator in the AI Core does not support float32 but supports only float16, the float32 AI CPU operator is used. If the AI CPU operator does not support it either, an error is reported.

- **allow\_mix\_precision/allow\_mix\_precision\_fp16:**

    When configured as allow\_mix\_precision or allow\_mix\_precision\_fp16, the effect is the same. Both indicate that the mixed precision data types float16, bfloat16, and float32 are used to process the neural network. allow\_mix\_precision\_fp16 is newly added in the new version and has clearer semantics for easier understanding.

    For operators of the float32 and bfloat16 data types in the original model, some float32 and bfloat16 operators are automatically reduced to float16 according to the built-in optimization policy, thereby improving system performance and reducing memory usage with minimal precision loss.

    If this mode is configured, you can view the value of the “precision\_reduce” parameter in the built-in optimization policy file `${INSTALL_DIR}/opp/built-in/op_impl/ai_core/tbe/config/xxx/aic-xxx-ops-info-*.json`:

  - If the value is true (trustlist), the current float32 and bfloat16 operators are allowed to reduce precision to float16.
  - If the value is false (blocklist), the current float32 and bfloat16 operators are not allowed to reduce precision to float16, and the corresponding operators still use float32 or bfloat16 precision.
  - If the operator in the network model does not have this parameter configured (graylist), the mixed precision processing mechanism of the current operator is consistent with that of the previous operator. That is, if the previous operator supports precision reduction, the current operator also supports precision reduction; if the previous operator does not allow precision reduction, the current operator also does not support precision reduction.

- **allow\_mix\_precision\_bf16:**

    Indicates the process of using mixed precision bfloat16 and float32 data types to process a neural network. For operators with the float32 data type in the original model, some float32 operators are automatically reduced to bfloat16 according to the built-in optimization policy, thereby improving system performance and reducing memory usage with minimal precision loss. If the operators on the AI Core do not support bfloat16 and float32, AI CPU operators are used for computation; if the AI CPU operators also do not support them, an error is reported during execution.

    If this mode is configured, you can view the value of the “precision\_reduce” parameter in the built-in optimization policy file `${INSTALL_DIR}/opp/built-in/op_impl/ai_core/tbe/config/xxx/aic-xxx-ops-info-*.json`:

  - If the value is true (trustlist), the current float32 operators are allowed to reduce precision to bfloat16.
  - If the value is false (blocklist), the current float32 operators are not allowed to reduce precision to bfloat16, and the corresponding operators still use float32 precision.
  - If the operator in the network model does not have this parameter configured (graylist), the mixed precision processing mechanism of the current operator is consistent with that of the previous operator. That is, if the previous operator supports precision reduction, the current operator also supports precision reduction; if the previous operator does not allow precision reduction, the current operator also does not support precision reduction.

- **allow\_fp32\_to\_bf16:**
  - If the operator precision in the original graph is float32, float32 is used first. If the operator on the AI Core does not support float32, the precision is reduced to bfloat16. If the operator on the AI Core does not support bfloat16, the AI CPU operator is used for computation. If the AI CPU operator does not support it either, an error is reported during execution.
  - If the operator precision in the original graph is bfloat16, bfloat16 is used first. If the operator on the AI Core does not support bfloat16, float32 is selected. If the operator on the AI Core does not support float32, the AI CPU operator is used for computation. If the AI CPU operator does not support it either, an error is reported during execution.

Replace $\{INSTALL\_DIR\} in the preceding path with the file storage path after CANN software installation. For example, when installed as the root user, the default file storage path is /usr/local/Ascend/cann.

**Parameter value constraints:**

<!-- @ref: ge/res/docs/en/api/graph_engine_api/cpp/ge/aclgrphBuildModel_config_params/precision_tuning_res.md#id1 -->
<!-- npu="950,A3,910b,310b" id3 -->
- **The bfloat16 data type is supported only on the following product types**:

    Atlas A2 products

    Atlas A3 products

    Atlas 200I/500 A2 inference products

    Ascend 950PR&950DT products
<!-- end id3 -->

- This parameter defaults to performance-first, which may cause precision overflow during subsequent inference. If precision issues occur during inference, see "Model Inference \> Precision/Performance Optimization \> Suggestions for Improving Model Inference Precision" in [Application Development](https://gitcode.com/cann/docs/blob/9.2.0/docs/zh/app-dev/c/00_acl_cpp_dev.md) for troubleshooting.
- If you focus on precision issues, you can change it to another value, such as **must\_keep\_origin\_dtype.**

**Configuration example:**

```c++
{ge::ir_option::PRECISION_MODE, "force_fp16"}
```

**Product support:**

Supported on all chips.

## PRECISION\_MODE\_V2

Sets the precision mode of operators. It cannot be used together with PRECISION\_MODE in the same graph. It is recommended to use the PRECISION\_MODE\_V2 parameter. The options parameter actually corresponding to this parameter is `ge.exec.precision_mode_v2`.

**Parameter values:**

- **fp16 (default value):**

    Indicates that when the operator precision in the original graph is float16, bfloat16, or float32, float16 is forcibly selected.

- **origin:**

    The original graph precision is maintained.

  - If the precision of an operator in the original graph is float16, and the implementation of the operator in the AI Core does not support float16 but supports only float32 and bfloat16, the system automatically uses the higher precision float32.
  - If the precision of an operator in the original graph is float16, and the implementation of the operator in the AI Core does not support float16 but supports only bfloat16, the float16 AI CPU operator is used. If the AI CPU operator does not support it either, an error is reported.
  - If the precision of an operator in the original graph is float32, and the implementation of the operator in the AI Core does not support float32 but supports only float16, the float32 AI CPU operator is used. If the AI CPU operator does not support it either, an error is reported.

- **cube\_fp16in\_fp32out:**

    When the operator in the AI Core supports both float32 and float16 data types, the system internally selects different processing methods based on the operator type.

  - For cube computation-type operators, the system internally processes them based on the support status of the operator implementation:

      1. Preferentially select the input data type as float16 and the output data type as float32;

      2. If the scenario in 1 is not supported, select the input data type as float32 and the output data type as float32;

      3. If the scenario in 2 is not supported, select the input data type as float16 and the output data type as float16;

      4. If the scenario in 3 is not supported, an error is reported.

  - For vector computation-type operators, indicates that when the operator precision in the original graph is float16 or bfloat16, float32 is forcibly selected.

      If the original graph contains some operators whose implementation on the AI Core does not support float32, for example, an operator that supports only float16, this parameter does not take effect and the supported float16 is still used. If the implementation of the operator on the AI Core does not support float32 and a blocklist is also configured (precision\_reduce = false), the float32 AI CPU operator is used. If the AI CPU operator is also unsupported, an error is reported during execution.

- **mixed\_float16:**

    Indicates that the neural network is processed using mixed precision float16, bfloat16, and float32 data types. For operators of float32 and bfloat16 data types in the original graph, some float32 and bfloat16 operators are automatically reduced to float16 precision according to the built-in optimization policy, thereby improving system performance and reducing memory usage with minimal precision loss.

    If this mode is configured, you can view the value of the “precision\_reduce” parameter in the built-in optimization policy file `${INSTALL_DIR}/opp/built-in/op_impl/ai_core/tbe/config/xxx/aic-xxx-ops-info-*.json`:

  - If the value is true (trustlist), the current float32 and bfloat16 operators are allowed to reduce precision to float16.
  - If the value is false (blocklist), the current float32 and bfloat16 operators are not allowed to reduce precision to float16, and the corresponding operators still use float32 or bfloat16 precision.
  - If the operator in the network model does not have this parameter configured (graylist), the mixed precision processing mechanism of the current operator is consistent with that of the previous operator. That is, if the previous operator supports precision reduction, the current operator also supports precision reduction; if the previous operator does not allow precision reduction, the current operator also does not support precision reduction.

- **mixed\_bfloat16:**

    Indicates that the neural network is processed using mixed precision bfloat16 and float32 data types. For operators of float32 data type in the original graph, some float32 operators are automatically reduced to bfloat16 precision according to the built-in optimization policy, thereby improving system performance and reducing memory usage with minimal precision loss. If an operator does not support bfloat16 and float32, the AI CPU operator is used for computation; if the AI CPU operator also does not support them, an error is reported during execution.

    If this mode is configured, you can view the value of the “precision\_reduce” parameter in the built-in optimization policy file `${INSTALL_DIR}/opp/built-in/op_impl/ai_core/tbe/config/xxx/aic-xxx-ops-info-*.json`:

  - If the value is true (trustlist), the current float32 operators are allowed to reduce precision to bfloat16.
  - If the value is false (blocklist), the current float32 operators are not allowed to reduce precision to bfloat16, and the corresponding operators still use float32 precision.
  - If the operator in the network model does not have this parameter configured (graylist), the mixed precision processing mechanism of the current operator is consistent with that of the previous operator. That is, if the previous operator supports precision reduction, the current operator also supports precision reduction; if the previous operator does not allow precision reduction, the current operator also does not support precision reduction.

- **mixed\_hif8:**

    Enables the automatic mixed precision feature, which uses a mix of hifloat8 (for an introduction to this data type, see [Link](https://arxiv.org/abs/2409.16626?context=cs.AR)), float16, bfloat16, and float32 data types to process neural networks. For operators of float16, bfloat16, and float32 data types in the original graph, some float16, bfloat16, and float32 operators are automatically reduced in precision to hifloat8 according to the built-in optimization policy, thereby improving system performance and reducing memory usage with minimal precision loss.

    If this mode is configured, you can view the value of the “**precision\_reduce**” parameter in the built-in optimization policy file at `${INSTALL_DIR}/opp/built-in/op_impl/ai_core/tbe/config/xxx/aic-xxx-ops-info-*.json`:

  - If the value is true (trustlist), the current float16, bfloat16, and float32 operators are allowed to be reduced in precision to hifloat8.
  - If the value is false (blocklist), the current float16, bfloat16, and float32 operators are not allowed to be reduced in precision to hifloat8, and the corresponding operators still use float16, bfloat16, or float32 precision.
  - If the operator in the original graph is not configured with this parameter (graylist), the mixed precision processing mechanism of the current operator remains consistent with that of the previous operator. That is, if the previous operator supports precision reduction, the current operator also supports precision reduction; if the previous operator does not allow precision reduction, the current operator does not support precision reduction either.

- **cube\_hif8:**

   Indicates that when a cube operator in the original graph supports both hifloat8 and float16, bfloat16, or float32 data types, the hifloat8 data type is forcibly selected.

Replace $\{INSTALL\_DIR\} in the preceding path with the file storage path after CANN software installation. For example, when installed as the root user, the default file storage path is /usr/local/Ascend/cann.

**Parameter value constraints:**

<!-- @ref: ge/res/docs/en/api/graph_engine_api/cpp/ge/aclgrphBuildModel_config_params/precision_tuning_res.md#id2 -->
<!-- npu="950,A3,910b,310b" id9 -->
- **The bfloat16 data type is supported only on the following product models**:

    <!-- npu="950" id10 -->
    Ascend 950PR&950DT products
    <!-- end id10 -->

    <!-- npu="A3" id11 -->
    Atlas A3 products
    <!-- end id11 -->

    <!-- npu="910b" id12 -->
    Atlas A2 products
    <!-- end id12 -->

    <!-- npu="310b" id13 -->
    Atlas 200I/500 A2 inference products
    <!-- end id13 -->

<!-- end id9 -->
<!-- npu="950" id8 -->
- **The hif8 data type is supported only on the following product models**:

  Ascend 950PR&950DT products
<!-- end id8 -->

- This parameter defaults to performance-first, which may cause precision overflow during subsequent inference. If precision issues occur during inference, see "Model Inference \> Precision/Performance Optimization \> Suggestions for Improving Model Inference Precision" in [Application Development](https://gitcode.com/cann/docs/blob/9.2.0/docs/zh/app-dev/c/00_acl_cpp_dev.md) for troubleshooting.
- If you focus on precision issues, you can change it to another value, such as **origin.**

**Configuration example:**

```c++
{ge::ir_option::PRECISION_MODE_V2, "fp16"}
```

**Applicable products:**

Supported on all chips.

## MODIFY\_MIXLIST

In mixed precision scenarios, this parameter specifies the path and file name of the mixed precision trustlist, blocklist, and graylist, allowing you to specify which operators are allowed to undergo precision reduction and which are not. The options parameter actually corresponding to this parameter is `ge.exec.modify_mixlist`.

**Parameter values:**

Configure the path and file name. The file is in JSON format. The trustlist, blocklist, and graylist can be viewed from the flag parameter value under the “precision\_reduce” parameter in the built-in optimization policy file `${INSTALL_DIR}/opp/built-in/op_impl/ai_core/tbe/config/xxx/aic-xxx-ops-info-*.json` (where $\{INSTALL\_DIR\} should be replaced with the file storage path after CANN software installation. For example, when installed as the root user, the file is stored by default at /usr/local/Ascend/cann. Select xxx based on the actual product.):

- If the value is true (trustlist), it indicates that precision reduction is **allowed** in mixed precision mode.
- If the value is false (blocklist), it indicates that precision reduction is **not allowed** in mixed precision mode.
- If this parameter is not configured (graylist), it indicates that in mixed precision mode, the mixed precision processing mechanism of the current operator remains consistent with that of the previous operator. That is, if the previous operator supports precision reduction, the current operator also supports precision reduction; if the previous operator does not allow precision reduction, the current operator also does not support precision reduction.

To enable mixed precision:

- Set the PRECISION\_MODE parameter to allow\_mix\_precision, allow\_mix\_precision\_fp16, and allow\_mix\_precision\_bf16.
- Set the PRECISION\_MODE\_V2 parameter to mixed\_float16, mixed\_bfloat16, and mixed\_hif8. It cannot be configured together with the PRECISION\_MODE parameter. It is recommended to use PRECISION\_MODE\_V2.

    <!-- npu="950,A3,910b,310b" id15 -->
    bf16 is supported only on the following product models:

    <!-- npu="950" id16 -->
    Ascend 950PR&950DT products
    <!-- end id16 -->

    <!-- npu="A3" id17 -->
    Atlas A3 products
    <!-- end id17 -->

    <!-- npu="910b" id18 -->
    Atlas A2 products
    <!-- end id18 -->

    <!-- npu="310b" id19 -->
    Atlas 200I/500 A2 inference products
    <!-- end id19 -->
    <!-- end id15 -->

    <!-- npu="950" id20 -->
    hif8 is supported only on the Ascend 950PR&950DT product series.
    <!-- end id20 -->
    <!-- @ref: ge/res/docs/en/api/graph_engine_api/cpp/ge/aclgrphBuildModel_config_params/precision_tuning_res.md#id3 -->

**Configuration example:**

```c++
{ge::ir_option::MODIFY_MIXLIST, "/home/test/ops_info.json"}
```

In ops\_info.json, you can specify operator types. Separate multiple operators with commas. An example is as follows:

```json
{
  "black-list": {                  // Blocklist
     "to-remove": [                // Convert blocklist operators to graylist operators. When configuring this parameter, ensure that the operators to be converted already exist in the blocklist.
     "Xlog1py"
     ],
     "to-add": [                   // Convert trustlist or graylist operators to blocklist operators.
     "Matmul",
     "Cast"
     ]
  },
  "white-list": {                  // Trustlist
     "to-remove": [                // Convert trustlist operators to graylist operators. When configuring this parameter, ensure that the operators to be converted already exist in the trustlist.
     "Conv2D"
     ],
     "to-add": [                   // Convert blocklist or graylist operators to trustlist operators.
     "Bias"
     ]
  }
}
```

The operators shown in the preceding configuration file example are for reference only. Configure them based on the actual hardware environment and the specific built-in operator optimization policies. The following shows an example of querying the trustlist, blocklist, and graylist:

```json
"Conv2D":{
    "precision_reduce":{
        "flag":"true"
     }
},
```

true: trustlist. false: blocklist. Not configured: graylist.

**Applicable products:**

Supported on all chips.

## CUSTOMIZE\_DTYPES

The options parameter actually corresponding to this parameter is `ge.customizeDtypes`.

This parameter customizes the computation precision of operators during model compilation. Other operators in the model are compiled with the precision mode specified by PRECISION\_MODE or PRECISION\_MODE\_V2. This parameter must be set to the path and file name of a configuration file, for example: /home/test/customize\_dtypes.cfg.

**Parameter value constraints:**

- The configuration file lists the operator names or operator types whose computation precision needs to be customized, with each operator on a separate line. The operator type must be the type of an operator defined based on IR.
- For the same operator, if both the operator name and the operator type are configured, the operator name takes precedence during compilation.
- When specifying the computation precision of an operator, if the operator is fused during compilation, the specified computation precision does not take effect.

Configuration file format requirements:

```text
# Configure by operator name
Opname1::InputDtype:dtype1,dtype2,…OutputDtype:dtype1,…
Opname2::InputDtype:dtype1,dtype2,…OutputDtype:dtype1,…
# Configure by operator type
OpType::TypeName1:InputDtype:dtype1,dtype2,…OutputDtype:dtype1,…
OpType::TypeName2:InputDtype:dtype1,dtype2,…OutputDtype:dtype1,…
```

Configuration file example:

```text
# Configure by operator name
resnet_v1_50/block1/unit_3/bottleneck_v1/Relu::InputDtype:float16,int8,OutputDtype:float16,int8
# Configure by operator type
OpType::Relu:InputDtype:float16,int8,OutputDtype:float16,int8
```

> [!NOTE]
>
> - The computation precisions supported by an operator can be viewed in the operator information library. The default storage path is `${INSTALL_DIR}/opp/built-in/op_impl/ai_core/tbe/config/xxx/aic-xxx-ops-info-*.json`.
>     Replace $\{INSTALL\_DIR\} with the file storage path after CANN software installation. For example, if you install CANN as the root user, the default storage path is /usr/local/Ascend/cann. Select xxx based on the actual product.
> - The priority specified by this parameter is high, which may cause a decrease in precision or performance. If the specified dtype is not supported, compilation fails.

**Configuration example:**

```c++
{ge::ir_option::CUSTOMIZE_DTYPES, "/home/test/customize_dtypes.cfg"}
```

**Applicable products:**

Supported on all chips.
