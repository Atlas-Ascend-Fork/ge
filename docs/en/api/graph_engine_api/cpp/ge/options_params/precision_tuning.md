# Precision Tuning

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-10-26T12:55:59.825Z pushedAt=2026-10-28T07:42:19.757Z nl=2 -->

## ge.customizeDtypes

This parameter customizes the computation precision of operators during model compilation. Other operators in the model are compiled with the precision mode specified by ge.exec.precision\_mode or ge.exec.precision\_mode\_v2. This parameter must be set to the path and file name of a configuration file, for example: /home/test/customize\_dtypes.cfg.

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
# Configure by operator name.
resnet_v1_50/block1/unit_3/bottleneck_v1/Relu::InputDtype:float16,int8,OutputDtype:float16,int8
# Configure by operator type.
OpType::Relu:InputDtype:float16,int8,OutputDtype:float16,int8
```

> [!NOTE]
>
> - The computation precisions supported by an operator can be viewed in the operator information library. The default storage path is `${INSTALL_DIR}/opp/built-in/op_impl/ai_core/tbe/config/xxx/aic-xxx-ops-info-*.json`. Replace $\{INSTALL\_DIR\} with the file storage path after CANN software installation. For example, if you install CANN as the root user, the default storage path is /usr/local/Ascend/cann. Select xxx based on the actual product.
> - The priority specified by this parameter is high, which may cause a decrease in precision or performance. If the specified dtype is not supported, compilation fails.

**Required/Optional**: Optional

**Effective level**: session

## ge.exec.modify\_mixlist

In mixed precision scenarios, this parameter specifies the path and file name of the mixed precision trustlist, blocklist, and graylist, allowing you to specify which operators are allowed to reduce precision and which are not. Configure it as a path and file name, where the file is in JSON format.

The trustlist, blocklist, and graylist can be viewed from the flag parameter value under the “precision\_reduce” parameter in the built-in optimization policy file `${INSTALL_DIR}/opp/built-in/op_impl/ai_core/tbe/config/xxx/aic-xxx-ops-info-*.json` (where $\{INSTALL\_DIR\} should be replaced with the file storage path after CANN software installation. For example, when installed as the root user, the file is stored by default at /usr/local/Ascend/cann. Replace xxx based on the actual product.):

- If the value is true (trustlist), it indicates that precision reduction is **allowed** in mixed precision mode.
- If the value is false (blocklist), it indicates that precision reduction is **not allowed** in mixed precision mode.
- If this parameter is not configured (graylist), it indicates that in mixed precision mode, the mixed precision processing mechanism of the current operator remains consistent with that of the previous operator. That is, if the previous operator supports precision reduction, the current operator also supports precision reduction; if the previous operator does not allow precision reduction, the current operator also does not support precision reduction.

**Configuration example:**

```json
{"ge.exec.modify_mixlist", "/home/test/ops_info.json"};
```

In ops\_info.json, you can specify operator types. Separate multiple operators with commas. An example is as follows:

```json
{
  "black-list": {                  // Blocklist
     "to-remove": [                // Convert blocklist operators to graylist operators
     "Xlog1py"
     ],
     "to-add": [                   // Convert trustlist or graylist operators to blocklist operators.
     "Matmul",
     "Cast"
     ]
  },
  "white-list": {                  // Trustlist.
     "to-remove": [                // Convert trustlist operators to graylist operators.
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

true: trustlist; false: blocklist; not configured: graylist.

**Required/Optional**: Optional

**Effective level**: Global/session/graph

## ge.exec.precision\_mode

Operator precision mode. The configuration value must be a string. It cannot be used together with ge.exec.precision\_mode\_v2. It is recommended to use the ge.exec.precision\_mode\_v2 parameter.

**Parameter values:**

- **force\_fp32/cube\_fp16in\_fp32out:**

    When configured as force\_fp32 or cube\_fp16in\_fp32out, the effect is the same. This option indicates that when the operator in the AI Core supports both float32 and float16 data types, the system internally selects different processing methods based on the operator type. cube\_fp16in\_fp32out is newly added in the new version, and for cube computation operators, this option has clearer semantics.

  - For cube computation-type operators, the system internally processes them based on the support status of the operator implementation:

      1. Preferentially select the input data type as float16 and the output data type as float32;

      2. If the scenario in 1 is not supported, select the input data type as float32 and the output data type as float32;

      3. If the scenario in 2 is not supported, select the input data type as float16 and the output data type as float16;

      4. If the scenario in 3 is not supported, an error is reported.

  - For vector computation-type operators, indicates that when the operator precision in the original graph is float16 or bfloat16, float32 is forcibly selected.

    If the original graph contains some operators whose implementation on the AI Core does not support float32, for example, an operator that supports only float16, this parameter does not take effect and the supported float16 is still used. If the implementation of the operator on the AI Core does not support float32 and a blocklist is also configured (precision\_reduce = false), the float32 AI CPU operator is used. If the AI CPU operator is also unsupported, an error is reported during execution.

- **force\_fp16:**

    Indicates that when the operator precision in the original graph is float16, bfloat16, or float32, float16 is forcibly selected.

- **allow\_fp32\_to\_fp16:**
  - For cube-type operators:
    - If the operator precision in the original graph is float32, precision is preferentially reduced to float16. If the operator on the AI Core does not support float16, float32 is selected instead. If the operator on the AI Core does not support float32, the AI CPU operator is used for computation. If the AI CPU operator is also unsupported, an error is reported.
    - If the operator precision in the original graph is bfloat16, the original precision bfloat16 is preferentially used. If the operator on the AI Core does not support bfloat16, float32 is selected. If the operator on the AI Core does not support float32, precision is directly reduced to float16. If the operator on the AI Core does not support float16, the AI CPU operator is used for computation. If the AI CPU operator is also unsupported, an error is reported.

  - For vector-type operators, the original precision is preferentially retained:
    - If the operator precision in the original graph is float32, the original precision float32 is preferentially used. If the operator on the AI Core does not support float32, precision is directly reduced to float16. If the operator on the AI Core does not support float16, the AI CPU operator is used for computation. If the AI CPU operator is also unsupported, an error is reported.
    - If the operator precision in the original graph is bfloat16, the original precision bfloat16 is used first. If the operator in the AI Core does not support bfloat16, float32 is selected. If the operator in the AI Core does not support float32, the precision is directly reduced to float16. If the operator in the AI Core does not support float16, the AI CPU operator is used for computation. If the AI CPU operator does not support it either, an error is reported.

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

In the preceding paths, replace $\{INSTALL\_DIR\} with the file storage path after CANN software installation. For example, when installing as the root user, the default file storage path after installation is /usr/local/Ascend/cann. Select xxx based on the actual product.

**Default values:**

<!-- npu="910" id1 -->
In the training scenario, for Atlas training series products, the default value is “allow\_fp32\_to\_fp16”.
<!-- end id1 -->

<!-- npu="910b" id2 -->
In the training scenario, for Atlas A2 series products, the default value is “must\_keep\_origin\_dtype”.
<!-- end id2 -->

<!-- npu="A3" id3 -->
In the training scenario, for Atlas A3 series products, the default value is “must\_keep\_origin\_dtype”.
<!-- end id3 -->

<!-- npu="950" id4 -->
In the training scenario, for Ascend 950PR&950DT series products, the default value is “must\_keep\_origin\_dtype”.
<!-- end id4 -->

In the online inference scenario, the default value is “force\_fp16”.

<!-- npu="950,A3,910b" id5 -->

**Parameter value constraints:**

**The bfloat16 data type is supported only on the following products**:

Atlas A2 products

Atlas A3 products

Ascend 950PR&950DT products
<!-- end id5 -->

**Configuration example:**

```c++
{"ge.exec.precision_mode", "force_fp16"};
```

**Required/Optional**: Optional

**Effective level**: Global/session/graph

## ge.exec.precision\_mode\_v2

Operator precision mode. The configuration must be of the string type. It cannot be used together with ge.exec.precision\_mode. It is recommended to use the ge.exec.precision\_mode\_v2 parameter.

**Parameter values:**

- **fp16**:

    Indicates that when the operator precision in the original graph is float16, bfloat16, or float32, float16 is forcibly selected.

- **origin**:

    Keeps the precision of the original graph.

  - If the precision of an operator in the original graph is float16, and the implementation of the operator in the AI Core does not support float16 but supports only float32 and bfloat16, the system internally adopts the high-precision float32 automatically.
  - If the precision of an operator in the original graph is float16, and the implementation of the operator in the AI Core does not support float16 but supports only bfloat16, the float16 AI CPU operator is used. If the AI CPU operator does not support it either, an error is reported during execution.
  - If the precision of an operator in the original graph is float32, and the implementation of the operator in the AI Core does not support the float32 type but supports only the float16 type, the float32 AI CPU operator is used. If the AI CPU operator does not support it either, an error is reported during execution.

- **cube\_fp16in\_fp32out:**

    When the operator in the AI Core supports both float32 and float16 data types, the system internally selects different processing methods based on the operator type.

  - For cube computation-type operators, the system internally processes them based on the support status of the operator implementation:

      1. Preferentially select the input data type float16 and the output data type float32;

      2. If the scenario in 1 is not supported, select the input data type float32 and the output data type float32;

      3. If the scenario in 2 is not supported, select the input data type float16 and the output data type float16;

      4. If the scenario in 3 is not supported, an error is reported.

  - For vector computation-type operators, it indicates that the operator precision in the original graph is float16 or bfloat16, and float32 is forcibly selected.

    If the original graph contains some operators whose implementations in the AI Core do not support float32, for example, an operator that supports only the float16 type, this parameter does not take effect and the supported float16 is still used; if the implementation of the operator in the AI Core does not support float32 and a blocklist is also configured (precision\_reduce = false), the float32 AI CPU operator is used; if the AI CPU operator is also not supported, an error is reported.

- **mixed\_float16:**

    Indicates that the neural network is processed using mixed precision float16, bfloat16, and float32 data types. For operators of float32 and bfloat16 data types in the original graph, some float32 and bfloat16 operators are automatically reduced to float16 precision according to the built-in optimization policy, thereby improving system performance and reducing memory usage with minimal precision loss.

    If this mode is configured, you can view the value of the “precision\_reduce” parameter in the built-in optimization policy file `${INSTALL_DIR}/opp/built-in/op_impl/ai_core/tbe/config/xxx/aic-xxx-ops-info-*.json`:

  - If the value is true (trustlist), the current float32 and bfloat16 operators are allowed to be reduced to float16 precision.
  - If the value is false (blocklist), the current float32 and bfloat16 operators are not allowed to be reduced to float16 precision, and the corresponding operators still use float32 or bfloat16 precision.
  - If the operator in the network model is not configured with this parameter (graylist), the mixed precision processing mechanism of the current operator remains consistent with that of the previous operator. That is, if the previous operator supports precision reduction, the current operator also supports precision reduction; if the previous operator does not allow precision reduction, the current operator also does not support precision reduction.

- **mixed\_bfloat16:**

    Indicates that the neural network is processed using mixed precision bfloat16 and float32 data types. For operators of float32 data type in the original graph, some float32 operators are automatically reduced to bfloat16 precision according to the built-in optimization policy, thereby improving system performance and reducing memory usage with minimal precision loss. If an operator does not support bfloat16 and float32, the AI CPU operator is used for computation; if the AI CPU operator also does not support them, an error is reported during execution.

    If this mode is configured, you can view the value of the “precision\_reduce” parameter in the built-in optimization policy file `${INSTALL_DIR}/opp/built-in/op_impl/ai_core/tbe/config/xxx/aic-xxx-ops-info-*.json`:

  - If the value is true (trustlist), the current float32 operators are allowed to be reduced to bfloat16 precision.
  - If the value is false (blocklist), the current float32 operators are not allowed to be reduced to bfloat16 precision, and the corresponding operators still use float32 precision.
  - If the operator in the network model is not configured with this parameter (graylist), the mixed precision processing mechanism of the current operator remains consistent with that of the previous operator. That is, if the previous operator supports precision reduction, the current operator also supports precision reduction; if the previous operator does not allow precision reduction, the current operator does not support precision reduction either.

- **mixed\_hif8:**

    Enables the automatic mixed precision feature, which uses a mix of hifloat8 (for an introduction to this data type, see [Link](https://arxiv.org/abs/2409.16626?context=cs.AR)), float16, bfloat16, and float32 data types to process neural networks. For operators of float16, bfloat16, and float32 data types in the original graph, some float16, bfloat16, and float32 operators are automatically reduced in precision to hifloat8 according to the built-in optimization policy, thereby improving system performance and reducing memory usage with minimal precision loss.

    If this mode is configured, you can view the value of the “**precision\_reduce**” parameter in the built-in optimization policy file at `${INSTALL_DIR}/opp/built-in/op_impl/ai_core/tbe/config/xxx/aic-xxx-ops-info-*.json`:

  - If the value is true (trustlist), the current float16, bfloat16, and float32 operators are allowed to be reduced in precision to hifloat8.
  - If the value is false (blocklist), the current float16, bfloat16, and float32 operators are not allowed to be reduced in precision to hifloat8, and the corresponding operators still use float16, bfloat16, or float32 precision.
  - If the operator in the original graph is not configured with this parameter (graylist), the mixed precision processing mechanism of the current operator remains consistent with that of the previous operator. That is, if the previous operator supports precision reduction, the current operator also supports precision reduction; if the previous operator does not allow precision reduction, the current operator does not support precision reduction either.

- **cube\_hif8:**

    Indicates that when a cube operator in the original graph supports both hifloat8 and float16, bfloat16, or float32 data types, the hifloat8 data type is forcibly selected.

In the preceding path, replace $\{INSTALL\_DIR\} with the file storage path after the CANN software is installed. For example, when installing as the root user, the default file storage path after installation is /usr/local/Ascend/cann. Select xxx based on the actual product.

**Default value:**

<!-- npu="910" id6 -->
In the training scenario, for Atlas training series products, this configuration item has no default value and follows the default value of the “precision\_mode” parameter, that is, “allow\_fp32\_to\_fp16”.
<!-- end id6 -->

<!-- npu="910b" id7 -->
In the training scenario, for Atlas A2 series products, the default value is "origin".
<!-- end id7 -->

<!-- npu="A3" id8 -->
In the training scenario, for Atlas A3 series products, the default value is "origin".
<!-- end id8 -->

<!-- npu="950" id9 -->
In the training scenario, for Ascend 950PR&950DT series products, the default value is "origin".
<!-- end id9 -->

In the online inference scenario, the default value is "fp16".

<!-- npu="950,A3,910b" id10 -->
**Parameter value constraints:**

**The bfloat16 data type is supported only on the following products**:

Atlas A2 series products

Atlas A3 series products

Ascend 950PR&950DT products
<!-- end id10 -->

**Configuration example:**

```c++
{"ge.exec.precision_mode_v2", "fp16"};
```

**Required/Optional**: Optional

**Effective level**: Global/session/graph
