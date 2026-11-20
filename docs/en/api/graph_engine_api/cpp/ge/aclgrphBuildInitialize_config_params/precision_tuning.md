# Precision Tuning

<!-- npu="950,A3,910b,910,310p,310b" id1 -->
<!-- md-trans-meta sourceCommit=a0ebfd022a9200d2cd068bee8ba395fc30eab4d9 translatedAt=2026-11-17T02:51:11.062Z pushedAt=2026-11-18T02:41:08.899Z nl=2 -->

## ALLOW\_HF32

Whether to enable the feature of automatically replacing the float32 data type with HF32. In the current version, this parameter takes effect only for Conv type operators and Matmul type operators. The options parameter actually corresponding to this parameter is `ge.exec.allow_hf32`.

HF32 is a single-precision floating-point type launched by Ascend specifically for internal computation within operators. Its comparison with other common data types is shown in the following figure. As can be seen, HF32 and float32 support the same value range, but the mantissa precision (11 bits) of HF32 is close to that of FP16 (10 bits). By reducing precision, the HF32 single-precision data type replaces the original float32 single-precision data type, which greatly reduces the space occupied by data and improves performance.

![Diagram](../../../figures/hf32.png)

**Parameter values:**

- true: For Conv type operators and Matmul type operators, enable the feature of automatically converting the FP32 data type to the HF32 data type.

    For details about which operators are specifically involved, see opp/built-in/op\_impl/ai\_core/tbe/impl\_mode/allow\_hf32\_matmul\_**t**\_conv\_**t**.ini in the file storage path after software installation. This file cannot be modified by users.

- false: For Conv type operators and Matmul type operators, disable the feature of automatically converting the FP32 data type to the HF32 data type.

    For details about which operators are specifically involved, see opp/built-in/op\_impl/ai\_core/tbe/impl\_mode/allow\_hf32\_matmul\_**f**\_conv\_**f**.ini in the file storage path after software installation. This file cannot be modified by users.

**Default parameter value:**
For Conv type operators, the conversion from FP32 to HF32 is enabled by default; for Matmul type operators, this conversion feature is disabled by default.

**Usage constraints:**

- For the same operator, if OP\_PRECISION\_MODE or enable\_float\_32\_execution is configured through the enable\_hi\_float\_32\_execution parameter, it is not recommended to use it together with the ALLOW\_HF32 parameter in this scenario. If they are used together, the priority is as follows:

    OP\_PRECISION\_MODE (ByNodeName, set the precision mode by node name) \> ALLOW\_HF32 \> OP\_PRECISION\_MODE (ByOpType, set the precision mode by operator type).

- Because ALLOW\_HF32 uses HF32 to automatically replace float32, for this parameter to take effect, the input or output type of the involved operator must be float32. Because the default value of the PRECISION\_MODE\_V2 parameter is fp16, when the operator type in the original network model is float32, it is forcibly converted to float16. In this scenario, the ALLOW\_HF32 parameter does not take effect. It is recommended to change the PRECISION\_MODE\_V2 parameter value to **origin** (the default value of the PRECISION\_MODE parameter is force\_fp16; it is recommended to change it to **must\_keep\_origin\_dtype** or **force\_fp32**).

**Configuration example:**

```c++
{ge::ir_option::ALLOW_HF32, "true"}
```

**Applicable products:**

<!-- npu="950" id2 -->
- Ascend 950PR&950DT products: Supported
<!-- end id2 -->
<!-- npu="A3" id31 -->
- Atlas A3 products: Supported
<!-- end id31 -->
<!-- npu="910b" id9 -->
- Atlas A2 products: Supported
<!-- end id9 -->
<!-- npu="310b" id10 -->
- Atlas 200I/500 A2 inference products: Not supported
<!-- end id10 -->
<!-- npu="310p" id11 -->
- Atlas inference products: Not supported
<!-- end id11 -->
<!-- npu="910" id30 -->
- Atlas training products: Not supported
<!-- end id30 -->

<!-- end id1 -->

## PRECISION\_MODE

Sets the precision mode of operators. It cannot be used together with **PRECISION\_MODE\_V2** in the same graph. It is recommended to use the **PRECISION\_MODE\_V2** parameter. The **options** parameter that actually corresponds to this parameter is `ge.exec.precision_mode`.

**Parameter values:**

- **force\_fp32/cube\_fp16in\_fp32out:**

    Configuring it as force\_fp32 or cube\_fp16in\_fp32out has the same effect. This option is used to indicate that when the operator in the AI Core supports both float32 and float16 data types, the system internal processing differs according to the operator type. cube\_fp16in\_fp32out is newly added in the new version. For cube computation-type operators, this option has clearer semantics.

  - For cube computation operators, the system handles them internally based on the operator implementation's support:

      1. Prioritize selecting the input data type as float16 and the output data type as float32;

      2. If the scenario in 1 is not supported, then select the input data type as float32 and the output data type as float32;

      3. If the scenario in 2 is not supported, then select the input data type as float16 and the output data type as float16;

      4. If the scenario in 3 is not supported, then an error is reported.

  - For vector computation-type operators, it indicates that the operator precision in the original graph is float16 or bfloat16, and float32 is forcibly selected.

      If some operators in the original graph have implementations in the AI Core that do not support float32, for example, an operator supports only the float16 type, this parameter does not take effect and the supported float16 is still used; if the implementation of the operator in the AI Core does not support float32 and a blocklist is also configured (precision\_reduce = false), then the float32 AI CPU operator is used; if the AI CPU operator also does not support it, then an error is reported.

- **force\_fp16 (default value):**

    Indicates that when the operator precision in the original graph is float16, bfloat16, or float32, float16 is forcibly selected.

- **allow\_fp32\_to\_fp16:**
  - For cube-type operators:
    - If the operator precision in the original graph is float32, precision is first reduced to float16; if the operator in the AI Core does not support float16, then float32 is selected; if the operator in the AI Core does not support float32, then the AI CPU operator is used for computation; if the AI CPU operator also does not support it, then an error is reported.
    - If the operator precision in the original graph is bfloat16, then the original graph precision bfloat16 is used first; if the operator in the AI Core does not support bfloat16, then float32 is selected; if the operator in the AI Core does not support float32, then precision is directly reduced to float16; if the operator in the AI Core does not support float16, then the AI CPU operator is used for computation; if the AI CPU operator also does not support it, then an error is reported.

  - For vector-type operators, the original graph precision is preferentially maintained:
    - If the operator precision in the original graph is float32, then the original graph precision float32 is used first; if the operator in the AI Core does not support float32, then precision is directly reduced to float16; if the operator in the AI Core does not support float16, then the AI CPU operator is used for computation; if the AI CPU operator also does not support it, then an error is reported.
    - If the operator precision in the original graph is bfloat16, prioritize using the original graph precision bfloat16. If the operator on the AI Core does not support bfloat16, then select float32. If the operator in the AI Core does not support float32, then directly reduce the precision to float16. If the operator in the AI Core does not support float16, then use the AI CPU operator for computation. If the AI CPU operator also does not support it, an error is reported.

- **must\_keep\_origin\_dtype:**

    Keep the original graph precision.

  - If a certain operator in the original graph has float16 precision, and the implementation of the operator in the AI Core does not support float16 but only supports float32 and bfloat16, then the system automatically adopts the high precision float32.
  - If a certain operator in the original graph has float16 precision, and the implementation of the operator in the AI Core does not support float16 but only supports bfloat16, then the float16 AI CPU operator is used. If the AI CPU operator also does not support it, then an error is reported.
  - If a certain operator in the original graph has float32 precision, and the implementation of the operator in the AI Core does not support the float32 type but only supports the float16 type, then the float32 AI CPU operator is used. If the AI CPU operator also does not support it, then an error is reported.

- **allow\_mix\_precision/allow\_mix\_precision\_fp16:**

    When configured as allow\_mix\_precision or allow\_mix\_precision\_fp16, the effect is the same: both indicate using mixed precision float16, bfloat16, and float32 data types to process the neural network. allow\_mix\_precision\_fp16 is newly added in the new version and has clearer semantics, making it easier to understand.

    For operators with float32 and bfloat16 data types in the original model, according to the built-in optimization strategy, some float32 and bfloat16 operators are automatically converted to lower precision float16, thus improving system performance and reducing memory usage with very small precision loss.

    If this mode is configured, then the parameter value of "precision\_reduce" can be viewed in the built-in optimization strategy file `${INSTALL_DIR}/opp/built-in/op_impl/ai_core/tbe/config/xxx/aic-xxx-ops-info-*.json`:

  - If the value is **true** (allowlist), it means allowing the current float32 and bfloat16 operators to reduce precision to float16.
  - If the value is **false** (blocklist), the current float32 and bfloat16 operators are not allowed to reduce precision to float16, and the corresponding operators still use float32 or bfloat16 precision.
  - If the operator in the network model does not have this parameter configured (graylist), the mixed precision processing mechanism of the current operator remains consistent with that of the previous operator. That is, if the previous operator supports precision reduction, the current operator also supports precision reduction; if the previous operator does not allow precision reduction, the current operator also does not support precision reduction.

- **allow\_mix\_precision\_bf16:**

    Indicates the process of using mixed precision bfloat16 and float32 data types to handle neural networks. For operators with the float32 data type in the original model, according to the built-in optimization strategy, some float32 operators are automatically converted to lower precision bfloat16, thus improving system performance and reducing memory usage with very small precision loss. If the operator in the AI Core does not support bfloat16 and float32, the AI CPU operator is used for computation; if the AI CPU operator also does not support them, then an error is reported.

    If this mode is configured, the value of the "precision\_reduce" parameter can be viewed in the built-in optimization strategy file `${INSTALL_DIR}/opp/built-in/op_impl/ai_core/tbe/config/xxx/aic-xxx-ops-info-*.json`:

  - If the value is **true** (allowlist), the current float32 operators are allowed to reduce precision to bfloat16.
  - If the value is **false** (blocklist), the current float32 operators are not allowed to reduce precision to bfloat16, and the corresponding operators still use float32 precision.
  - If the operator in the network model does not have this parameter configured (graylist), the mixed precision processing mechanism of the current operator remains consistent with that of the previous operator. That is, if the previous operator supports precision reduction, the current operator also supports precision reduction; if the previous operator does not allow precision reduction, the current operator also does not support precision reduction.

- **allow\_fp32\_to\_bf16:**
  - If the operator precision in the original graph is float32, prioritize using the original graph precision float32. If the operator in the AI Core does not support float32, reduce the precision to bfloat16. If the operator in the AI Core does not support bfloat16, use the AI CPU operator for computation. If the AI CPU operator also does not support it, an error is reported.
  - If the operator precision in the original graph is bfloat16, prioritize using the original graph precision bfloat16. If the operator in the AI Core does not support bfloat16, select float32. If the operator in the AI Core does not support float32, use the AI CPU operator for computation. If the AI CPU operator also does not support it, an error is reported.

Replace **$\{INSTALL\_DIR\}** in the preceding path with the file storage path after the CANN software is installed. For example, if you install the software as the root user, the default file storage path after installation is **/usr/local/Ascend/cann**.

**Parameter value constraints:**

<!-- npu="IPV350" id8 -->
- **IPV350 does not support the bfloat16 type, and the corresponding option is also not supported.**
<!-- end id8 -->
<!-- npu="950,A3,910b,310b" id3 -->
- **The bfloat16 data type is supported only on the following product types**:

    <!-- npu="950" id4 -->
    Ascend 950PR&950DT series products
    <!-- end id4 -->

    <!-- npu="A3" id5 -->
    Atlas A3 series products
    <!-- end id5 -->

    <!-- npu="910b" id6 -->
    Atlas A2 series products
    <!-- end id6 -->

    <!-- npu="310b" id7 -->
    Atlas 200I/500 A2 inference products
    <!-- end id7 -->
<!-- end id3 -->
<!-- @ref: ge/res/docs/en/api/graph_engine_api/cpp/ge/aclgrphBuildInitialize_config_params/precision_tuning_res.md#id1 -->

- This parameter prioritizes performance by default, which may cause precision overflow issues during subsequent inference. If precision issues occur during inference, see "Model Inference \> Precision/Performance Optimization \> Suggestions for Improving Model Inference Precision" in *[Application Development](https://gitcode.com/cann/docs/blob/master/docs/zh/app-dev/c/00_acl_cpp_dev.md)* for troubleshooting.
- If users focus on precision issues, they can change it to other values, such as **must\_keep\_origin\_dtype.**

**Configuration example:**

```c++
{ge::ir_option::PRECISION_MODE, "force_fp16"}
```

**Applicable products:**

Supported on all chips.

## PRECISION\_MODE\_V2

Sets the precision mode of operators. It cannot be used together with **PRECISION\_MODE** in the same graph. It is recommended to use the **PRECISION\_MODE\_V2** parameter. The **options** parameter that this parameter actually corresponds to is `ge.exec.precision_mode_v2`.

**Parameter values:**

- **fp16 (default value):**

    Indicates that when the operator precision in the original graph is float16, bfloat16, or float32, float16 is forcibly selected.

- **origin:**

    Keeps the original graph precision.

  - If the precision of a certain operator in the original graph is float16, and the implementation of the operator in the AI Core does not support float16 but supports only float32 and bfloat16, the system internally automatically uses the higher precision float32.
  - If the precision of a certain operator in the original graph is float16, and the implementation of the operator in the AI Core does not support float16 but supports only bfloat16, the float16 AI CPU operator is used; if the AI CPU operator also does not support it, an error is reported during execution.
  - If the precision of a certain operator in the original graph is float32, and the implementation of the operator in the AI Core does not support the float32 type but supports only the float16 type, the float32 AI CPU operator is used; if the AI CPU operator also does not support it, an error is reported during execution.

- **cube\_fp16in\_fp32out:**

    When the operator in the AI Core supports both float32 and float16 data types, the system internally selects different processing methods based on the operator type.

  - For cube computation-type operators, the system internally processes them according to the support status of the operator implementation:

      1. Prioritize selecting the input data type as float16 and the output data type as float32;

      2. If the scenario in 1 is not supported, select the input data type as float32 and the output data type as float32;

      3. If the scenario in 2 is not supported, select the input data type as float16 and the output data type as float16;

      4. If the scenario in 3 is not supported, an error is reported.

  - For vector computation-type operators, it indicates that the operator precision in the original graph is float16 or bfloat16, and float32 is forcibly selected.

      If some operators exist in the original graph and the implementation of the operator in the AI Core does not support float32, for example, a certain operator supports only the float16 type, then this parameter does not take effect and the supported float16 is still used; if the implementation of the operator in the AI Core does not support float32 and a blocklist is also configured (precision\_reduce = false), then the float32 AI CPU operator is used; if the AI CPU operator is also not supported, an error is reported.

- **mixed\_float16:**

    Indicates that mixed precision with float16, bfloat16, and float32 data types is used to process the neural network. For operators with float32 and bfloat16 data types in the original graph, some float32 and bfloat16 operators are automatically reduced to float16 precision according to the built-in optimization strategy, thereby improving system performance and reducing memory usage with very small precision loss.

    If this mode is configured, you can view the value of the "precision\_reduce" parameter in the built-in optimization strategy file `$`{INSTALL_DIR}/opp/built-in/op_impl/ai_core/tbe/config/xxx/aic-xxx-ops-info-*.json`:

  - If the value is **true** (allowlist), it means that the current float32 and bfloat16 operators are allowed to be reduced to float16 precision.
  - If the value is **false** (blocklist), the current float32 and bfloat16 operators are not allowed to be reduced to float16 precision, and the corresponding operators still use float32 or bfloat16 precision.
  - If the operator in the network model does not have this parameter configured (graylist), the mixed precision processing mechanism of the current operator remains consistent with that of the previous operator. That is, if the previous operator supports precision reduction, the current operator also supports precision reduction; if the previous operator does not allow precision reduction, the current operator also does not support precision reduction.

- **mixed\_bfloat16:**

    Indicates that mixed precision with bfloat16 and float32 data types is used to process the neural network. For operators with float32 data type in the original graph, some float32 operators are automatically reduced to bfloat16 precision according to the built-in optimization strategy, thereby improving system performance and reducing memory usage with very small precision loss. If an operator does not support bfloat16 and float32, the AI CPU operator is used for computation; if the AI CPU operator also does not support them, an error is reported.

    If this mode is configured, you can view the value of the "precision\_reduce" parameter in the built-in optimization strategy file `${INSTALL_DIR}/opp/built-in/op_impl/ai_core/tbe/config/xxx/aic-xxx-ops-info-*.json`:

  - If the value is **true** (allowlist), it means that the current float32 operators are allowed to be reduced to bfloat16 precision.
  - If the value is **false** (blocklist), the current float32 operators are not allowed to be reduced to bfloat16 precision, and the corresponding operators still use float32 precision.
  - If the operator in the network model is not configured with this parameter (graylist), the mixed precision processing mechanism of the current operator remains consistent with that of the previous operator. That is, if the previous operator supports precision reduction, the current operator also supports precision reduction; if the previous operator does not allow precision reduction, the current operator also does not support precision reduction.

- **mixed\_hif8:**

    Enables the automatic mixed precision function, which means mixing hifloat8 (for an introduction to this data type, see [Link](https://arxiv.org/abs/2409.16626?context=cs.AR)), float16, bfloat16, and float32 data types to process neural networks. For operators of float16, bfloat16, and float32 data types in the original graph, according to the built-in optimization strategy, some float16, bfloat16, and float32 operators are automatically reduced in precision to hifloat8, thus improving system performance and reducing memory usage with very small precision loss.

    If this mode is configured, you can view the value of the "**precision\_reduce**" parameter in the built-in optimization strategy file `${INSTALL_DIR}/opp/built-in/op_impl/ai_core/tbe/config/xxx/aic-xxx-ops-info-*.json`:

  - If the value is **true** (allowlist), it means allowing the current float16, bfloat16, and float32 operators to be reduced in precision to hifloat8.
  - If the value is **false** (blocklist), the current float16, bfloat16, and float32 operators are not allowed to be reduced in precision to hifloat8, and the corresponding operators still use float16, bfloat16, or float32 precision.
  - If the operator in the original graph is not configured with this parameter (graylist), the mixed precision processing mechanism of the current operator remains consistent with that of the previous operator. That is, if the previous operator supports precision reduction, the current operator also supports precision reduction; if the previous operator does not allow precision reduction, the current operator also does not support precision reduction.

- **cube\_hif8:**

   Indicates that when a cube operator in the original graph supports both hifloat8 and float16, bfloat16, or float32 data types, the hifloat8 data type is forcibly selected.

Replace **$\{INSTALL\_DIR\}** in the preceding path with the file storage path after CANN software installation. Taking installation by the root user as an example, the default file storage path after installation is **/usr/local/Ascend/cann**.

**Parameter value constraints:**

<!-- @ref: ge/res/docs/en/api/graph_engine_api/cpp/ge/aclgrphBuildInitialize_config_params/precision_tuning_res.md#id2 -->

<!-- npu="IPV350" id14 -->
- **IPV350 does not support bfloat16 or hif8 types, and the corresponding options are also not supported.**
<!-- end id14 -->
<!-- npu="950,A3,910b,310b" id12 -->
- **The bfloat16 data type is supported only on the following product models**:

    Ascend 950PR&950DT series products

    Atlas A3 series products

    Atlas A2 series products

    Atlas 200I/500 A2 inference products

<!-- end id12 -->

<!-- npu="950" id13 -->
- **The hif8 data type is supported only on the following product models**:

    Ascend 950PR&950DT series products
<!-- end id13 -->

- This parameter defaults to performance-first, which may cause precision overflow issues during subsequent inference. If precision issues occur during inference, see "Model Inference \> Precision/Performance Optimization \> Model Inference Precision Improvement Suggestions" in [*Application Development*](https://gitcode.com/cann/docs/blob/master/docs/zh/app-dev/c/00_acl_cpp_dev.md) for troubleshooting.
- If users focus on precision issues, they can change it to another value, such as **origin.**

**Configuration example:**

```c++
{ge::ir_option::PRECISION_MODE_V2, "fp16"}
```

**Applicable products:**

Supported on all chips.

## MODIFY\_MIXLIST

In mixed precision scenarios, this parameter specifies the path and file name of the mixed precision allowlist/blocklist/graylist, allowing you to specify which operators are allowed to reduce precision and which are not. The **options** parameter actually corresponding to this parameter is `ge.exec.modify_mixlist`.

**Parameter values:**

Configure it as a path and file name, where the file is in JSON format. The allowlist/blocklist/graylist can be viewed from the flag parameter value of the "precision\_reduce" parameter in the built-in optimization strategy file `${INSTALL_DIR}/opp/built-in/op_impl/ai_core/tbe/config/xxx/aic-xxx-ops-info-*.json` (where **$\{INSTALL\_DIR\}** should be replaced with the file storage path after CANN software installation. Taking root user installation as an example, the default file storage path after installation is: **/usr/local/Ascend/cann**. xxx should be selected based on the actual product.):

- If the value is **true** (allowlist), it means that in mixed precision mode, precision reduction is **allowed**.
- If the value is **false** (blocklist), it means that in mixed precision mode, precision reduction is **not allowed**.
- If this parameter is not configured (graylist), it means that in mixed precision mode, the mixed precision processing mechanism of the current operator remains consistent with that of the previous operator. That is, if the previous operator supports precision reduction, the current operator also supports precision reduction; if the previous operator does not allow precision reduction, the current operator also does not support precision reduction.

Ways to enable mixed precision:

- Set the **PRECISION\_MODE** parameter to **allow\_mix\_precision**, **allow\_mix\_precision\_fp16**, and **allow\_mix\_precision\_bf16**.
- Set the **PRECISION\_MODE\_V2** parameter to **mixed\_float16**, **mixed\_bfloat16**, and **mixed\_hif8**. It cannot be configured together with the **PRECISION\_MODE** parameter. It is recommended to use **PRECISION\_MODE\_V2**.

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

    <!-- @ref: ge/res/docs/en/api/graph_engine_api/cpp/ge/aclgrphBuildInitialize_config_params/precision_tuning_res.md#id3 -->

**Configuration example:**

```c++
{ge::ir_option::MODIFY_MIXLIST, "/home/test/ops_info.json"}
```

**ops\_info.json** allows you to specify operator types. Multiple operators are separated by commas. An example is as follows:

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
  "white-list": {                  // Trustlist.
     "to-remove": [                // Convert allowlist operators to graylist operators. When configuring this parameter, ensure that the operators to be converted already exist in the allowlist.
     "Conv2D"
     ],
     "to-add": [                   // Convert blocklist or graylist operators to allowlist operators.
     "Bias"
     ]
  }
}
```

The operators shown in the configuration file sample above are for reference only. Configure them based on the actual hardware environment and the specific built-in operator optimization strategy. A sample query of the allowlist, blocklist, and graylist is as follows:

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
