# aclCompileOpt

<!-- md-trans-meta sourceCommit=607806abd035eaf2ab464a02634ae7b347df3984 translatedAt=2026-10-27T14:35:13.560Z pushedAt=2026-10-29T08:14:30.664Z nl=2 -->

## Definition

```c
typedef enum {
    ACL_PRECISION_MODE,           // Precision mode of operators in the network model.
    ACL_AICORE_NUM,               // Number of AI Cores used for model compilation.
    ACL_AUTO_TUNE_MODE,           // Automatic tuning mode of operators.
    ACL_OP_SELECT_IMPL_MODE,      // Selects whether operators use high-precision or high-performance implementations.
    ACL_OPTYPELIST_FOR_IMPLMODE,  // Lists operator types. Operators in this list use the mode specified by ACL_OP_SELECT_IMPL_MODE.
    ACL_OP_DEBUG_LEVEL,           // Switch for the TBE operator compilation debug function.
    ACL_DEBUG_DIR,                // Path for saving debug-related process files generated during operator compilation in model conversion and network migration, including operator .o/.json/.cce files.
    ACL_OP_COMPILER_CACHE_MODE,   // Disk cache mode for operator compilation.
    ACL_OP_COMPILER_CACHE_DIR,    // Directory for the operator compilation disk cache.
    ACL_OP_PERFORMANCE_MODE,      // Sets whether to compile operators in high-performance execution mode.
    ACL_OP_JIT_COMPILE,           // Selects whether to compile operators online or use precompiled operator binary files.
    ACL_OP_DETERMINISTIC,         // Enables or disables deterministic computation.
    ACL_CUSTOMIZE_DTYPES,         // Customizes the computation precision of one or more operators during model compilation.
    ACL_OP_PRECISION_MODE,        // Specifies the precision mode for internal operator processing. Supports specifying one or more operators.
    ACL_ALLOW_HF32,               // hf32 is a precision type introduced by AI processors specifically for internal operator computation.
    ACL_PRECISION_MODE_V2,        // Precision mode for operators in the network model. Compared with the ACL_PRECISION_MODE option, ACL_PRECISION_MODE_V2 is newly added in the new version, offering more precision modes and clearer semantics for the original precision mode option values.
    ACL_OP_DEBUG_OPTION           // Currently supports only the value oom, which enables Global Memory out-of-bounds access detection.
} aclCompileOpt;
```

## ACL\_PRECISION\_MODE Value Description

Used for Configuration of the operator precision mode of the network model. If Not Configured, Default Adoption is allow\_fp32\_to\_fp16.

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

    <!-- npu="950,A3,910b,310b" id1 -->
- **allow\_mix\_precision\_bf16:**

    Indicates the process of using mixed precision bfloat16 and float32 data types to process a neural network. For operators of the float32 data type in the original model, some float32 operators are automatically reduced to bfloat16 according to the built-in optimization strategy, thereby improving system performance and reducing memory usage with minimal precision loss. If the operator on the AI Core does not support bfloat16 and float32, the AI CPU operator is used for computation; if the AI CPU operator does not support them either, an error is reported.

    If this mode is configured, you can view the value of the “precision\_reduce” parameter in the built-in optimization strategy file `${INSTALL_DIR}/opp/built-in/op_impl/ai_core/tbe/config/xxx/aic-xxx-ops-info-*.json`:

    - If the value is true (trustlist), the current float32 operator is allowed to reduce precision to bfloat16.
    - If the value is false (blocklist), the current float32 operator is not allowed to reduce precision to bfloat16, and the corresponding operator still uses float32 precision.
    - If the operator in the network model is not configured with this parameter (graylist), the mixed precision processing mechanism of the current operator remains consistent with that of the previous operator. That is, if the previous operator supports precision reduction, the current operator also supports precision reduction; if the previous operator does not allow precision reduction, the current operator does not support precision reduction either.
    <!-- end id1 -->

    <!-- npu="950,A3,910b,310b" id2 -->
- **allow\_fp32\_to\_bf16:**
    - If the operator precision in the original image is float32, the original image precision float32 is used preferentially. If the operator on the AI Core does not support float32, the precision is reduced to bfloat16; if the operator on the AI Core does not support bfloat16, the AI CPU operator is used for computation; if the AI CPU operator does not support it either, an error is reported.
    - If the operator precision in the original image is bfloat16, the original image precision bfloat16 is used first. If the operator on the AI Core does not support bfloat16, float32 is selected. If the operator on the AI Core does not support float32, the AI CPU operator is used for computation. If the AI CPU operator does not support it either, an error is reported during execution.
    <!-- end id2 -->

## ACL\_AICORE\_NUM Value Description

Used to configure the AI Core count used during model compilation.

<!-- npu="950,A3,910b,910,310p,310b" id3 -->
This setting does not take effect in the current version.
<!-- end id3 -->

<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclCompileOpt_res.md#id1 -->

## ACL\_AUTO\_TUNE\_MODE Value Description

Used for Configuration of the auto-tuning mode for operators. **This parameter is deprecated. Do not configure it, as compatibility issues may arise in later versions.**

## ACL\_OP\_SELECT\_IMPL\_MODE Value Description

Used to select whether the operator uses a high-precision implementation or a high-performance implementation. If this compilation option is not configured, high\_precision is adopted by default.

- **high\_precision**: indicates that operators use the high-precision implementation mode.

    This option uses the system built-in configuration file to set the operator implementation mode. The built-in configuration file path is $\{INSTALL\_DIR\}/opp/built-in/op\_impl/ai\_core/tbe/impl\_mode/high\_precision.ini.

    For compatibility, this parameter takes effect only on the operator list in the high\_precision.ini file. This list controls the scope of operators that take effect and ensures that network models of earlier versions are not affected.

- **high\_performance**: (default value) indicates that operators use the high-performance implementation mode.

    This option uses the system built-in configuration file to set the operator implementation mode. The built-in configuration file path is $\{INSTALL\_DIR\}/opp/built-in/op\_impl/ai\_core/tbe/impl\_mode/high\_performance.ini.

    For compatibility, this parameter takes effect only on the operator list in the high\_performance.ini file. This list controls the scope of operators that take effect and ensures that network models of earlier versions are not affected.

- **high\_precision\_for\_all**: indicates that operators use the high-precision implementation mode.

    This option uses the system built-in configuration file to set the operator implementation mode. The built-in configuration file path is $\{INSTALL\_DIR\}/opp/built-in/op\_impl/ai\_core/tbe/impl\_mode/high\_precision\_for\_all.ini, and the list in this file may be updated in later versions.

    **This implementation mode does not guarantee compatibility.** If an operator adds an implementation mode in a later software package (that is, an implementation mode of an operator is added to the configuration file), the performance of network models that use high\_precision\_for\_all in earlier versions may degrade in the new version.

- **high\_performance\_for\_all**: Indicates that the operator uses the high-performance implementation mode.

    This option uses the system built-in configuration file to set the operator implementation mode. The built-in configuration file path is $\{INSTALL\_DIR\}/opp/built-in/op\_impl/ai\_core/tbe/impl\_mode/high\_performance\_for\_all.ini, and the list in this file may be updated in later versions.

    **This implementation mode does not guarantee compatibility.** If an operator adds an implementation mode in a later software package (that is, an implementation mode for an operator is added to the configuration file), the precision of a network model that uses high\_performance\_for\_all in an earlier version may decrease in the new version.

## ACL\_OPTYPELIST\_FOR\_IMPLMODE Value Description

Use the ACL\_OPTYPELIST\_FOR\_IMPLMODE option to set the list of operator types (separate multiple operators with commas), and use it together with the ACL\_OP\_SELECT\_IMPL\_MODE option to set the operators in the list to be implemented with high precision or high performance.

## ACL\_OP\_DEBUG\_LEVEL Value Description

Used to configure the TBE operator debug function switch.

**The value description is as follows:**

- 0: (Default value) Disables the operator debug function and does **not generate** the operator compilation directory kernel\_meta in the current execution path.
- 1: Enables the operator debug function, generates the kernel\_meta folder in the current execution path, and **generates **\*.o (operator binary file), the \*.json file (operator description file), and the TBE instruction mapping file (operator cce file \*.cce and python-cce mapping file \*\_loc.json)** in the folder, for subsequent analysis of AI Core Error issues.

    <!-- npu="950" id5 -->
    Ascend 950PR&950DT products do not generate the TBE instruction mapping file.
    <!-- end id5 -->

- 2: Enables the operator debug function, generates the kernel\_meta folder in the current execution path, and **generates **\*.o (operator binary file), the \*.json file (operator description file), and the TBE instruction mapping file (operator cce file \*.cce and python-cce mapping file \*\_loc.json)** in the folder, for subsequent analysis of AI Core Error issues. Setting the value to 2 also disables the compilation optimization switch and enables the ccec debug function (the ccec compiler option is set to -O0-g).

    <!-- npu="950" id6 -->
    Ascend 950PR&950DT products do not generate the TBE instruction mapping file.
    <!-- end id6 -->

- 3: Disables the operator debug function, generates the kernel\_meta folder in the current execution path, and **generates **\*.o (operator binary file) and the \*.json file (operator description file)** in the folder, which can be referenced when analyzing operator issues.
- 4: Disables the operator debug function, generates the kernel\_meta folder in the current execution path, and **generates **\*.o (operator binary file), the \*.json file (operator description file), the TBE instruction mapping file (operator cce file \*.cce), and the UB fusion computation description file (\{$kernel\_name\}\_compute.json)** in the folder, which can be used for issue reproduction and precision comparison when analyzing operator issues.

    <!-- npu="950" id7 -->
    Ascend 950PR&950DT products do not generate the TBE instruction mapping file or the UB fusion computation description file.
    <!-- end id7 -->

**Configuration constraints are as follows:**

- When set to 2 (that is, the ccec compilation option is enabled), the operator Kernel (\*.o file) size increases. In dynamic Shape scenarios, because the operator compilation traverses possible Shape scenarios, the operator Kernel file may become too large to compile. In this case, it is recommended that you do not configure the ccec compilation option.

    The following is an example of an error log indicating that the operator Kernel file is too large to compile:

    ```bash
    message:link error ld.lld: error: InputSection too large for range extension thunk ./kernel_meta_xxxxx.o
    ```

- When the debug function switch is enabled, if the model contains the following general computation fusion operators, the \*.o, \*.json, and \*.cce files of the following operators are not generated in the operator compilation directory kernel\_meta.

    MatMulAllReduce

    MatMulAllReduceAddRmsNorm

    AllGatherMatMul

    MatMulReduceScatter

    AlltoAllAllGatherBatchMatMul

    BatchMatMulReduceScatterAlltoAll

## ACL\_DEBUG\_DIR Value Description

Used for configuring the path for saving debug-related process files generated during operator compilation in model conversion and network migration (default path: the current path where the application is executed /kernel\_meta), including operator .o/.json/.cce files. The specific files generated depend on the value set by the ACL\_OP\_DEBUG\_LEVEL option.

The path supports uppercase and lowercase letters (a-z, A-Z), digits (0-9), underscores (\_), hyphens (-), periods (.), and Chinese characters.

For configuring the cache directory of operator compilation files, in addition to setting the enumeration value here, you can also configure the environment variable ASCEND\_WORK\_PATH. The priority of these methods is: setting the enumeration value in this section \> setting the environment variable \> default path. For detailed configuration of the environment variable, see *Environment Variable Reference*.

## ACL\_OP\_COMPILER\_CACHE\_MODE Value Description

Used for configuring the operator compilation disk cache mode. This compilation option must be used together with ACL\_OP\_COMPILER\_CACHE\_DIR.

- enable: enables the operator build cache. After it is enabled, repeated builds of operators with the same build parameters and operator parameters can be avoided, thereby improving the build speed.
- force: enables the operator build cache. Different from the enable mode, the force mode forcibly refreshes the cache, that is, deletes the existing cache first, and then rebuilds and adds the cache. For example, when the user's Python changes, the dependency libraries change, or the knowledge base changes after operator tuning, you need to first set the mode to force to clear the existing cache, and then change it to enable to avoid forcibly refreshing the cache each time a build is performed.
- disable: (default value) disables the operator build cache, and operators are rebuilt.

If the debug switch is also enabled (the ACL\_OP\_DEBUG\_LEVEL option is configured to a non-zero value), the system ignores the configuration of the ACL\_OP\_COMPILER\_CACHE\_MODE option and does not cache the compilation results of debug scenarios.

When the operator compilation cache function is enabled, you can set the disk space size of the cache folder in two ways: through a **configuration file** (the op\_cache.ini configuration file is automatically generated in the path specified by the ACL\_OP\_COMPILER\_CACHE\_DIR option during operator compilation) or through **environment variables**:

1. Set through the configuration file op\_cache.ini

    If the op\_cache.ini file does not exist, create it manually. Open the file and add the following information:

    ```json
    #Configuration file format, which must be included. The automatically generated file includes the following information by default. When creating it manually, you need to enter it.
    [op_compiler_cache]
    #Limit the disk space size of the cache folder under a certain chip. The default value is 500, the value must be an integer, and the unit is MB.
    max_op_cache_size=500
    #Set the ratio of cache space to be retained. Value range: [1,100]. The default value is 50, and the unit is percentage. For example, a value of 80 means that when the cache space is insufficient, cache files are deleted and 80% of the cache space is retained.
    remain_cache_size_ratio=50
    ```

    - The op\_cache.ini file takes effect only when the values of both the max\_op\_cache\_size and remain\_cache\_size\_ratio parameters in the preceding file are valid.
    - When the size of the compilation cache file exceeds the value set by “max\_op\_cache\_size” and the cache file has not been accessed for more than half an hour, the cache file is aged (during operator build, the process is not interrupted because the compilation cache file size exceeds the set value. Therefore, when “max\_op\_cache\_size” is set too small, the actual compilation cache file size may exceed the set value).
    - To disable the compilation cache aging function, set “max\_op\_cache\_size” to "-1". In this case, the access time is not updated when the operator cache is accessed, the operator compilation cache is not aged, and the default disk space size of 500 MB is used.
    - If multiple users use the same cache path, it is recommended to configure it through a configuration file. In this scenario, the op\_cache.ini file affects all users.

2. Configure through environment variables

    In this scenario, developers can use the environment variable ASCEND\_MAX\_OP\_CACHE\_SIZE to limit the disk space size of the cache folder for a chip. When the compilation cache space size reaches the value set by ASCEND\_MAX\_OP\_CACHE\_SIZE and the cache file has not been accessed for more than half an hour, the cache file is aged. The environment variable ASCEND\_REMAIN\_CACHE\_SIZE\_RATIO can be used to set the ratio of cache space to be retained.

    The following is a configuration example:

    ```bash
    # The default value of the ASCEND_MAX_OP_CACHE_SIZE environment variable is 500, the value must be an integer, and the unit is MB.
    export ASCEND_MAX_OP_CACHE_SIZE=500
    # Value range of the ASCEND_REMAIN_CACHE_SIZE_RATIO environment variable: [1,100]. The default value is 50, and the unit is percentage. For example, a value of 80 means that when the cache space is insufficient, cache files are deleted and 80% of the cache space is retained.
    export ASCEND_REMAIN_CACHE_SIZE_RATIO=50
    ```

    - Configuration through environment variables takes effect only for the current user.
    - To disable the compilation cache aging function, set the environment variable “**ASCEND\_MAX\_OP\_CACHE\_SIZE**” to "-1". In this case, the access time is not updated when the operator cache is accessed, the operator compilation cache is not aged, and the default disk space size of 500 MB is used.

**If both the op\_cache.ini file and environment variables are configured, the configuration items in the op\_cache.ini file are read first. If neither the op\_cache.ini file nor the environment variables are set, the system default values are read: the default disk space size is 500 MB, and 50% of the cache space is retained by default.**

## ACL\_OP\_COMPILER\_CACHE\_DIR Value Description

Used to configure the cache directory for operator compilation files (default path: $HOME/atc\_data). This compilation option must be used together with ACL\_OP\_COMPILER\_CACHE\_MODE.

The path supports uppercase and lowercase letters (a-z, A-Z), digits (0-9), underscores (\_), hyphens (-), periods (.), and Chinese characters.

If the ACL\_OP\_DEBUG\_LEVEL compilation option is set, the compilation cache feature is enabled only when the option value is 0; other values disable the compilation cache feature.

In addition to setting the enum value here, you can also configure the environment variable ASCEND\_CACHE\_PATH to specify the cache directory for operator compilation files. The priority of these methods is: setting the enum value in this section \> setting the environment variable \> the default path. For detailed configuration of the environment variable, see [*Environment Variable Reference*](https://gitcode.com/cann/docs/blob/9.2.0/docs/en/env-vars/README.md).

## ACL\_OP\_PERFORMANCE\_MODE Value Description

This option determines whether to compile operators in a way that prioritizes operator execution performance. **This parameter is deprecated. Do not configure it; otherwise, compatibility issues may arise in later versions.**

Value range:

- normal: Operators are compiled in the way that yields the highest compilation performance. This is the default.
- high: Operators are compiled through generalized compilation in the way that yields the best operator execution performance.

## ACL\_OP\_JIT\_COMPILE Value Description

Selects whether to compile operators online or use precompiled operator binary files.

Value range:

- enable: Compiles operators online. The system optimizes based on the obtained operator information to compile operators with better runtime performance. In fixed-shape network scenarios, it is recommended to set this option to enable.
- disable: Prioritizes searching for precompiled operator binary files in the system. If found, the operators are not compiled again, resulting in better compilation performance. If not found, the operators are compiled. In dynamic-shape network scenarios, it is recommended to set this option to disable.

The default value varies by product model:

<!-- npu="950" id8 -->
For Ascend 950PR&950DT products, the default value of this option is disable.
<!-- end id8 -->

<!-- npu="A3" id9 -->
For Atlas A3 products, the default value of this option is disable.
<!-- end id9 -->

<!-- npu="910b" id10 -->
For Atlas A2 products, the default value of this option is disable.
<!-- end id10 -->

<!-- npu="310b" id11 -->
For Atlas 200I/500 A2 inference products, the default value of this option is enable.
<!-- end id11 -->

<!-- npu="910" id12 -->
For Atlas training products, the default value of this option is enable.
<!-- end id12 -->

<!-- npu="310p" id13 -->
For Atlas inference products, the default value of this option is enable.
<!-- end id13 -->

<!-- npu="950,A3,910b,910,310p,310b" id17 -->
If this parameter is set to disable, install the operator binary package. For details, see [CANN Software Installation](https://www.hiascend.com/document/detail/en/CANNCommunityEdition/920/softwareinst/instg/instg_0000.html?OS=openEuler&InstallType=netyum).
<!-- end id17 -->

<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclCompileOpt_res.md#id2 -->

## ACL\_OP\_DETERMINISTIC Value Description

Whether to enable deterministic computation.

- 0: Default value. Deterministic computation is not enabled. When this option is configured, an operator may produce different results across multiple executions on the same hardware and with the same input. This difference generally arises because the operator implementation involves asynchronous multi-threaded execution, which changes the order of floating-point accumulation.
- 1: Deterministic computation is enabled. When this option is configured, an operator produces the same output across multiple executions on the same hardware and with the same input. However, enabling deterministic computation often slows down operator execution.

It is generally recommended not to enable deterministic computation, because deterministic computation often slows down operator execution and thus affects performance. When you find that a model produces different results across multiple executions, or when you are performing precision tuning, you can enable deterministic computation to assist with model debugging and tuning.

## ACL\_CUSTOMIZE\_DTYPES Value Description

\*.cfg Path of the configuration file, including the file name. The configuration file lists the operator names or operator types for which the computation precision needs to be specified, with one operator per line. With this configuration, you can customize the computation precision of one or more operators during model compilation.

**Configuration constraints:**

- The path and file name support uppercase and lowercase letters (a-z, A-Z), digits (0-9), underscores (\_), hyphens (-), periods (.), English colons \(:\), and Chinese characters.
- If the operator name is used in the configuration file, configure it in the **Opname::InputDtype:dtype1,...,OutputDtype:dtype1,...** format, with one Opname per line. dtype1, dtype2.. must correspond one-to-one to the operator inputs and outputs for which the computation precision can be set **.**
- If the operator type is used in the configuration file, configure it in the **OpType::TypeName:InputDtype:dtype1,...,OutputDtype:dtype1,...** format, with one OpType per line. dtype1, dtype2.. must correspond one-to-one to the operator inputs and outputs for which the computation precision can be set, and the operator OpType must be the OpType of an operator defined based on Ascend IR. For how to view the OpType, see "FAQ \> [How to Determine the Correspondence Between Operators in the Original Framework Network Model and Operators Supported by the AI Processor](../../../../user_guides/atc_tools/FAQ/operator_correspondence_guide.md)" in *[ATC Offline Model Compilation Tool](../../../../user_guides/atc_tools/README.md)*.
- For the same operator, if both the **Opname** and **OpType** configuration items are configured, the **Opname** configuration item takes effect during compilation.
- When this parameter is used to specify the computation precision of an operator, if the operator is fused during model conversion, the specified computation precision of the operator does not take effect.

## ACL\_OP\_PRECISION\_MODE

Sets the path and file name of the operator precision mode configuration file (.ini format). The path and file name support uppercase and lowercase letters (a-z, A-Z), digits (0-9), underscores (\_), hyphens (-), periods (.), and Chinese characters.

The following precision modes can be set in the configuration file:

- high\_precision: indicates high precision.
- high\_performance: indicates high performance.
    <!-- npu="A3,910b" id14 -->
- enable\_float\_32\_execution: uses the FP32 data type inside the operator. In this scenario, the FP32 data type is not automatically converted to the HF32 data type. If the precision loss exceeds expectations when using HF32 for computation, you can enable this configuration to specify that some operators use FP32 for internal computation to maintain precision.

    **This option is supported only on Atlas A2  products and Atlas A3  products.**

    <!-- end id14 -->
- enable\_hi\_float\_32\_execution: uses the HF32 data type inside the operator. After this configuration is enabled, the FP32 data type is automatically converted to the HF32 data type. This mechanism reduces the space occupied by data and improves performance.

    **This option is not supported in the current version.**

- support\_out\_of\_bound\_index: Indicates that out-of-bounds check is performed on the indices input of gather, scatter, and segment operators. The check reduces operator execution performance.
- keep\_fp16: Indicates that the FP16 data type is used inside the operator. In this scenario, the FP16 data type is not automatically converted to the FP32 data type. If the performance does not meet expectations when FP32 is used for computation and the precision requirement is not high, you can select the keep\_fp16 mode, which **sacrifices precision for performance. This low-precision mode is not recommended**.
- super\_performance: Indicates ultra-high performance. Compared with high performance, the algorithm calculation formula is optimized.

Create an operator precision mode configuration file _op\_precision.ini_, and set the precision mode in the file by operator type and node name. Set the precision mode for one operator type or node name on each line. The precision mode set by node name takes precedence over that set by operator type.

The following is a configuration example:

```json
[ByOpType]
optype1=high_precision
optype2=high_performance
optype3=support_of_bound_index

[ByNodeName]
nodename1=high_precision
nodename2=high_performance
nodename3=support_of_bound_index
```

## ACL\_ALLOW\_HF32 Value Description

Whether HF32 is allowed to replace FP32 in internal operator computation. true allows the replacement, and false disallows it. In the current version, this configuration takes effect only for Conv-type and Matmul-type operators. By default, FP32-to-HF32 conversion is enabled for Conv-type operators and disabled for Matmul-type operators.

HF32 is a single-precision floating-point type launched by the AI processor specifically for internal operator computation. The following figure compares HF32 with other common data types. HF32 supports the same value range as FP32, but its mantissa precision (11 bits) is close to that of FP16 (10 bits). By reducing precision, HF32 single-precision data type replaces the original FP32 single-precision data type, which greatly reduces the space occupied by data and improves performance.

![](figures/en-us_image_0000002550155921.png)

## ACL\_PRECISION\_MODE\_V2 Value Description

Precision mode of the operators in the network model. If this compilation option is not configured, fp16 is adopted by default.

Compared with the ACL\_PRECISION\_MODE option, ACL\_PRECISION\_MODE\_V2 is newly added in the new version, and its precision mode option values have clearer semantics and are easier to understand.

- **fp16 (default):**

    Indicates that when the operator precision in the original graph is float16, bfloat16, or float32, float16 is force-selected.

- **origin:**

    Keep the original image precision.

    - If the precision of an operator in the original image is float16, and the implementation of this operator on the AI Core does not support float16 but only supports float32 and bfloat16, the system automatically uses the higher precision float32.
    - If the precision of an operator in the original image is float16, and the implementation of this operator on the AI Core does not support float16 but only supports bfloat16, the float16 AI CPU operator is used. If the AI CPU operator is also not supported, an error is reported.
    - If the precision of an operator in the original image is float32, and the implementation of this operator on the AI Core does not support the float32 type but only supports the float16 type, the float32 AI CPU operator is used. If the AI CPU operator is also not supported, an error is reported.

- **cube\_fp16in\_fp32out:**

    When the operator on the AI Core supports both float32 and float16 data types, the system selects different processing methods based on the operator type.

    - For matrix computation operators, the system processes them based on the support status of the operator implementation:
        1. Preferentially select float16 as the input data type and float32 as the output data type;
        2. If the scenario in 1 is not supported, select float32 as the input data type and float32 as the output data type;
        3. If the scenario in 2 is not supported, select float16 as the input data type and float16 as the output data type;
        4. If the scenario in 3 is not supported, an error is reported.

    - For vector computation operators, it indicates that the operator precision in the original image is float16 or bfloat16, and float32 is forcibly selected.

        If some operators exist in the original image and the implementation of these operators on the AI Core does not support float32, for example, an operator only supports the float16 type, this parameter does not take effect and the supported float16 is still used. If the implementation of these operators on the AI Core does not support float32 and a blocklist is also configured (precision\_reduce = false), the float32 AI CPU operators are used. If the AI CPU operators are also not supported, an error is reported during execution.

- **mixed\_float16:**

    Indicates that mixed precision float16, bfloat16, and float32 data types are used to process the neural network. For operators with float32 and bfloat16 data types in the original graph, some float32 and bfloat16 operators are automatically reduced in precision to float16 according to the built-in optimization strategy, thereby improving system performance and reducing memory usage with minimal precision loss.

    If this mode is configured, you can view the value of the “precision\_reduce” parameter in the built-in optimization strategy file `${INSTALL_DIR}/opp/built-in/op_impl/ai_core/tbe/config/xxx/aic-xxx-ops-info-*.json`:

    - If the value is true (trustlist), it indicates that the current float32 and bfloat16 operators are allowed to be reduced to float16 precision.
    - If the value is false (blocklist), the current float32 and bfloat16 operators are not allowed to reduce precision to float16, and the corresponding operators still use float32 or bfloat16 precision.
    - If the operator in the network model is not configured with this parameter (graylist), the mixed precision processing mechanism of the current operator remains consistent with that of the previous operator. That is, if the previous operator supports precision reduction, the current operator also supports precision reduction; if the previous operator does not allow precision reduction, the current operator does not support precision reduction either.

    <!-- npu="950,A3,910b,310b" id15 -->
- **mixed\_bfloat16:**

    Indicates that mixed precision bfloat16 and float32 data types are used to process the neural network. For operators of the float32 data type in the original graph, some float32 operators are automatically reduced in precision to bfloat16 according to the built-in optimization strategy, thereby improving system performance and reducing memory usage with minimal precision loss. If an operator does not support bfloat16 and float32, an AI CPU operator is used for computation. If the AI CPU operator does not support them either, an error is reported.

    If this mode is configured, you can view the value of the “precision\_reduce” parameter in the built-in optimization strategy file `${INSTALL_DIR}/opp/built-in/op_impl/ai_core/tbe/config/xxx/aic-xxx-ops-info-*.json`:

    - If the value is true (trustlist), the current float32 operator is allowed to reduce precision to bfloat16.
    - If the value is false (blocklist), the current float32 operator is not allowed to reduce precision to bfloat16, and the corresponding operator still uses float32 precision.
    - If the operator in the network model is not configured with this parameter (graylist), the mixed precision processing mechanism of the current operator remains consistent with that of the previous operator. That is, if the previous operator supports precision reduction, the current operator also supports precision reduction; if the previous operator does not allow precision reduction, the current operator does not support precision reduction either.
    <!-- end id15 -->

    <!-- npu="950" id16 -->
- **mixed\_hif8:**

    Enables the automatic mixed precision function, which means that hifloat8 (for details about this data type, see [Link](https://arxiv.org/abs/2409.16626?context=cs.AR)), float16, bfloat16, and float32 data types are used together to process the neural network. For operators of the float16, bfloat16, and float32 data types in the original graph, some float16, bfloat16, and float32 operators are automatically reduced in precision to hifloat8 according to the built-in optimization strategy, thereby improving system performance and reducing memory usage with minimal precision loss.

    If this mode is configured, you can view the value of the “**precision\_reduce**” parameter in the built-in optimization strategy file `${INSTALL_DIR}/opp/built-in/op_impl/ai_core/tbe/config/xxx/aic-xxx-ops-info-*.json`:

    - If the value is true (trustlist), the current float16, bfloat16, and float32 operators are allowed to be reduced in precision to hifloat8.
    - If the value is false (blocklist), the current float16, bfloat16, and float32 operators are not allowed to be reduced in precision to hifloat8, and the corresponding operators still use float16, bfloat16, or float32 precision.
    - If this parameter is not configured for the operators in the original graph (graylist), the mixed precision processing mechanism of the current operator is consistent with that of the previous operator. That is, if the previous operator supports precision reduction, the current operator also supports precision reduction; if the previous operator does not allow precision reduction, the current operator does not support precision reduction either.

- **cube\_hif8:**

    Indicates that when the cube operators in the original graph support both hifloat8 and float16, bfloat16, or float32 data types, the hifloat8 data type is forcibly selected.
    <!-- end id16 -->

## ACL\_OP\_DEBUG\_OPTION Value Description

Currently, it can only be configured as oom, which enables Global Memory out-of-bounds access detection during the operator compilation phase.

Before compiling an operator, call the aclSetCompileopt API to set ACL\_OP\_DEBUG\_OPTION to oom, and also call the aclrtCtxSetSysParamOpt API (scoped to a Context) or the aclrtSetSysParamOpt API (scoped to a process) to set ACL\_OPT\_ENABLE\_DEBUG\_KERNEL to 1, thereby enabling Global Memory out-of-bounds access detection. During operator execution, if reading from or writing to Global Memory (for example, reading operator input data or writing operator output data) causes an out-of-bounds access, the error code "EZ9999" is returned, indicating an AI Core Error in the operator.
