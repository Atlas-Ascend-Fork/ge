# Operator and Graph Compilation

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-10-16T11:37:32.768Z pushedAt=2026-10-28T07:40:22.198Z nl=2 -->

## AICORE\_NUM

Configures the number of AI Cores used during operator compilation. The corresponding **options** parameter of this parameter is `ge.aicoreNum`.

**Parameter values**: "integer1|integer2", separated by "|":

<!-- npu="950,A3,910b" id1 -->
- **Scenario 1**: For the following products, integer1 indicates the number of Cube Cores in the AI Core used during operator compilation, and integer2 indicates the number of Vector Cores in the AI Core used during operator compilation. Both integer1 and integer2 must be greater than or equal to 0 and less than or equal to the maximum number of Cube Cores and Vector Cores contained in the AI processor:

    <!-- npu="950" id2 -->
    Ascend 950PR&950DT products
    <!-- end id2 -->

    <!-- npu="A3" id3 -->
    Atlas A3 products
    <!-- end id3 -->

    <!-- npu="910b" id4 -->
    Atlas A2 products
    <!-- end id4 -->
<!-- end id1 -->

<!-- npu="910,310p,310b" id5 -->
- **Scenario 2**: For the following products, only integer1 needs to be configured, in the format "integer1|". Configuring integer2 does not take effect. It indicates the number of AI Cores used during operator compilation:

    <!-- npu="310b" id6 -->
    Atlas 200I/500 A2 inference products
    <!-- end id6 -->

    <!-- npu="310p" id7 -->
    Atlas inference products
    <!-- end id7 -->

    <!-- npu="910" id8 -->
    Atlas training products
    <!-- end id8 -->

<!-- end id5 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/cpp/ge/aclgrphBuildInitialize_config_params/operator_and_graph_compilation_res.md#id1 -->

**Constraints:**

<!-- npu="950,A3,910b" id10 -->
- For scenario 1 in the parameter value:

    The maximum numbers of Cube Cores and Vector Cores on different AI processors can be viewed in the `${INSTALL_DIR}/<arch>-linux/data/platform_config/xxx.ini` file. As shown below, the AI processor has 24 Cube Cores and 48 Vector Cores.

    ```text
    [SoCInfo]
    # Configure the parameter to the default value, which is the maximum value.
    ai_core_cnt=24
    cube_core_cnt=24
    vector_core_cnt=48
    ```
<!-- end id10 -->

<!-- npu="910,310p,310b" id11 -->
- For scenario 2 in the parameter value:

    The maximum number of AI Cores on different AI processors can be viewed in the `${INSTALL_DIR}/<arch>-linux/data/platform_config/xxx.ini` file. As shown below, the AI processor has 10 AI Cores.

    ```text
    [SoCInfo]
    # Use the default AI Core value, which is the maximum value.
    ai_core_cnt=10
    vector_core_cnt=8
    ```
<!-- end id11 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/cpp/ge/aclgrphBuildInitialize_config_params/operator_and_graph_compilation_res.md#id2 -->
- Configuring the value to **0** does not cause an error, but it is treated as unconfigured and does not control the cores. For example, configuring "integer 1|0" is equivalent to configuring "integer 1|".
- If this parameter is configured while the operator compilation cache feature is enabled (the **OP\_COMPILER\_CACHE\_MODE** parameter is configured as **"enable"** or **"force"**, and defaults to **"enable"**), this parameter takes effect only during the first compilation. To make this parameter take effect in non-first compilations, you need to clear the compilation disk cache.

Here, **$\{INSTALL\_DIR\}** should be replaced with the file storage path after CANN software installation. Taking root user installation as an example, the default file storage path after installation is: **/usr/local/Ascend/cann**. **<arch\>** indicates the specific operating system architecture, and xxx should be selected based on the actual product.

**Configuration example:**

<!-- npu="950,A3,910b" id12 -->
- Configuration example for scenario 1:

    ```c++
    {ge::ir_option::AICORE_NUM, "24|48"}
    ```
<!-- end id12 -->

<!-- npu="910,310p,310b" id13 -->
- Configuration example for scenario 2

    ```c++
    {ge::ir_option::AICORE_NUM, "10|"}
    or
    {ge::ir_option::AICORE_NUM, "10"}
    ```
<!-- end id13 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/cpp/ge/aclgrphBuildInitialize_config_params/operator_and_graph_compilation_res.md#id3 -->

**Applicable products:**

Supported on all chips.

**Understanding the relationship among AI Core, Cube Core, and Vector Core:**

To facilitate understanding the relationship among AI Core, Cube Core, and Vector Core, the definition of Core is first clarified here. A Core refers to a compute core that has an independent Scalar compute unit. The Scalar compute unit typically handles functions such as SIMD (Single Instruction Multiple Data) instruction issue for a compute core, so this Scalar compute unit is also commonly referred to as the scheduling unit within the core. The AI data processing core units differ across products, and are currently classified into the following categories:

- When the AI data processing core unit is AI Core:
  - Within an AI Core, Cube and Vector share a single Scalar scheduling unit.
    <!-- npu="910" id14 -->
    For example, Atlas training products.
    <!-- end id14 -->

    ![Illustration](../../../figures/logic_arch_diagram.png)

  - Within an AI Core, Cube and Vector each have their own Scalar scheduling unit, so they are also called Cube Core and Vector Core. In this case, one Cube Core and a group of Vector Cores are defined as one AI Core, and the number of AI Cores is usually calculated based on how many Cube Cores there are.
    <!-- npu="910b" id15 -->
     For example, Atlas A2 series products.
     <!-- end id15 -->

- When the AI data processing core unit is an AI Core plus a separate Vector Core: both the AI Core and the Vector Core have independent Scalar scheduling units.
    <!-- npu="310p" id16 -->
    For example, Atlas inference products.
    <!-- end id16 -->

    ![](../../../figures/logic_arch_diagram_0.png)

## CORE\_TYPE

Sets the Core type used during graph compilation. If the graph contains Cube operators, only AiCore can be used. The **options** parameter actually corresponding to this parameter is `ge.engineType`.

**Parameter values:**

- VectorCore
- AiCore, which is the default value.

**Configuration example:**

```c++
{ge::ir_option::CORE_TYPE, "AiCore"}
```

**Applicable products:**

<!-- npu="950" id17 -->
- Ascend 950PR&950DT products: Not supported
<!-- end id17 -->
<!-- npu="A3" id18 -->
- Atlas A3 products: Not supported
<!-- end id18 -->
<!-- npu="910b" id19 -->
- Atlas A2 products: Not supported
<!-- end id19 -->
<!-- npu="310b" id20 -->
- Atlas 200I/500 A2 inference product: Not supported
<!-- end id20 -->
<!-- npu="310p" id21 -->
- Atlas inference products: Supported
<!-- end id21 -->
<!-- npu="910" id22 -->
- Atlas training products: Not supported
<!-- end id22 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/cpp/ge/aclgrphBuildInitialize_config_params/operator_and_graph_compilation_res.md#id4 -->

## OP\_COMPILER\_CACHE\_MODE

Configures the disk cache mode for operator compilation. The actual **options** parameter corresponding to this parameter is `ge.op_compiler_cache_mode`.

**Parameter values:**

- **enable**: (Default value) Enables the operator compilation cache. After it is enabled, repeated builds of operators with the same build parameters and operator parameters can be avoided, thereby improving the build speed.
- **force**: Enables the operator compilation cache. Different from the **enable** mode, the **force** mode forcibly refreshes the cache, that is, it deletes the existing cache first and then rebuilds and adds the result to the cache. For example, when the user's Python changes, dependent libraries change, or the repository changes after operator tuning, you need to first set the mode to **force** to clear the existing cache, and then change it to **enable** to avoid forcibly refreshing the cache on every build.
- **disable**: Disables the operator compilation cache, and operators are rebuilt.

**Configuration example:**

```c++
{ge::ir_option::OP_COMPILER_CACHE_MODE, "enable"}
```

**Usage notes:**

- To specify the path of the operator compilation disk cache, use it together with **OP\_COMPILER\_CACHE\_DIR**.
- When the operator compilation cache is enabled, you can set the disk space size of the cache folder in either of the following two ways: through the **configuration file** (after the operator compilation is complete, an **op\_cache.ini** file is automatically generated in the path specified by the **OP\_COMPILER\_CACHE\_DIR** parameter) or through the **environment variable**:

    1. Set through the configuration file op\_cache.ini

        If the op\_cache.ini file does not exist, create it manually. Open the file and add the following information:

        ```ini
        # Configuration file format, which must be included. The automatically generated file includes the following information by default, and it must be entered when creating the file manually.
        [op_compiler_cache]
        # Limit the disk space size of the cache folder under a chip. The default value is 500, the value must be an integer, and the unit is MB.
        max_op_cache_size=500
        # Set the proportion of cache space to be retained. Value range: [1,100], default value: 50, unit: percentage. For example, a value of 80 means that when the cache space is insufficient, cache files are deleted and 80% of the cache space is retained.
        remain_cache_size_ratio=50
        ```

        - The op\_cache.ini file takes effect only when the values of both the max\_op\_cache\_size and remain\_cache\_size\_ratio parameters are valid.
        - When the compilation cache file size exceeds the value set by “max\_op\_cache\_size” and the cache file has not been accessed for more than half an hour, the cache file is aged. (During operator compilation, the process is not interrupted because the compilation cache file size exceeds the set value. Therefore, when “max\_op\_cache\_size” is set too small, the actual compilation cache file size may exceed this set value.)
        - To disable the compilation cache aging function, set “max\_op\_cache\_size” to "-1". In this case, the access time is not updated when the operator cache is accessed, the operator compilation cache is not aged, and the default disk space size of 500 MB is used.
        - If multiple users use the same cache path, it is recommended to configure it through the configuration file. In this scenario, the op\_cache.ini file affects all users.

    2. Set through environment variables

        In this scenario, developers can use the environment variable ASCEND\_MAX\_OP\_CACHE\_SIZE to limit the disk space size of the cache folder under a specific chip. When the compilation cache space reaches the value set by ASCEND\_MAX\_OP\_CACHE\_SIZE and the cache file has not been accessed for more than half an hour, the cache file is aged out. The environment variable ASCEND\_REMAIN\_CACHE\_SIZE\_RATIO can be used to set the proportion of cache space to be retained.

        The configuration example is as follows:

        ```bash
        # The default value of the ASCEND_MAX_OP_CACHE_SIZE environment variable is 500. The value must be an integer, in MB.
        export ASCEND_MAX_OP_CACHE_SIZE=500
        # The value range of the ASCEND_REMAIN_CACHE_SIZE_RATIO environment variable is [1,100], and the default value is 50, in percentage. For example, a value of 80 means that when the cache space is insufficient, cache files are deleted and 80% of the cache space is retained.
        export ASCEND_REMAIN_CACHE_SIZE_RATIO=50
        ```

        - Configuration through environment variables takes effect only for the current user.
        - To disable the compilation cache aging function, set the environment variable “**ASCEND\_MAX\_OP\_CACHE\_SIZE**” to "-1". In this case, the access time is not updated when the operator cache is accessed, the operator compilation cache is not aged out, and the default disk space size of 500 MB is used.

    **If both the op\_cache.ini file and the environment variable are configured, the configuration items in the op\_cache.ini file are read first. If neither the op\_cache.ini file nor the environment variable is set, the system default values are read: the default disk space size is 500 MB, and 50% of the cache space is retained by default.**

- Because the **force** option deletes the existing cache first, it is not recommended to set it during parallel program compilation. Otherwise, other models may fail to compile because the cache content they use is cleared.
- It is recommended to set the compilation cache option to **disable** or **force** when the model is finally released.
- If the repository changes after operator tuning, set the option to **force** to refresh the cache. Otherwise, the new tuning repository cannot be applied, causing the tuning application to fail.
- Scenarios where the debug switch is enabled:
  - **OP\_DEBUG\_LEVEL** configured with a non-zero value: The configuration of the **OP\_COMPILER\_CACHE\_MODE** parameter is ignored, the operator compilation cache feature is not enabled, and all operators are recompiled.
  - **OP\_DEBUG\_CONFIG** configured with a non-empty value, and the **OP\_DEBUG\_LIST field is not configured**: The configuration of the **OP\_COMPILER\_CACHE\_MODE** parameter is ignored, the operator compilation cache feature is not enabled, and all operators are recompiled.
  - **OP\_DEBUG\_CONFIG** configured with a non-empty value, and the **OP\_DEBUG\_LIST field is configured in the configuration file**:
    - For operators in the list, the configuration of the OP\_COMPILER\_CACHE\_MODE parameter is ignored and they are recompiled.
    - For operators not in the list, if the OP\_COMPILER\_CACHE\_MODE parameter is configured as enable or force, the cache feature is enabled; if it is configured as disable, the cache feature is not enabled and they are still recompiled.

**Applicable products:**

Supported on all chips.

## OP\_COMPILER\_CACHE\_DIR

Configures the directory for the operator compilation disk cache. The actual **options** parameter corresponding to this parameter is `ge.op_compiler_cache_dir`.

**Parameter value format:**
The path supports uppercase and lowercase letters (a-z, A-Z), digits (0-9), underscores (\_), hyphens (-), periods (.), and Chinese characters.

**Default value:**
$HOME/atc\_data

**Configuration example:**

```c++
{ge::ir_option::OP_COMPILER_CACHE_MODE, "enable"},
{ge::ir_option::OP_COMPILER_CACHE_DIR, "/home/test/data/atc_data"}
```

**Constraints:**

- To specify a custom path for the operator compilation disk cache, this parameter must be used together with **OP\_COMPILER\_CACHE\_MODE**.
- If the path specified by this parameter exists and is valid, a subdirectory **kernel\_cache** is automatically created under the specified path. If the specified path does not exist but is valid, the directory is automatically created first, and then a subdirectory **kernel\_cache** is automatically created under that path.
- Do not store other user content in the **default cache directory**. User content is deleted together with the default cache directory when the software package is installed or upgraded.
- A **non-default cache directory** specified by this parameter cannot be deleted (it is not deleted when the software package is installed or upgraded).
- For the operator compilation disk cache path, in addition to the method of setting the **OP\_COMPILER\_CACHE\_DIR** parameter, you can also configure the environment variable **ASCEND\_CACHE\_PATH**. The priority of these methods is: configuration parameter "OP\_COMPILER\_CACHE\_DIR" > environment variable ASCEND\_CACHE\_PATH > default storage path. For details about the environment variable **ASCEND\_CACHE\_PATH**, see [*Environment Variable Reference*](https://gitcode.com/cann/docs/blob/9.2.0/docs/en/env-vars/README.md).

**Applicable products:**

Supported on all chips.

## OPTIMIZATION\_SWITCH

During operator compilation, this is the control switch for fusion patterns (Pass). The actual **options** parameter corresponding to this parameter is `ge.optimizationSwitch`.

The difference between this parameter and the **FUSION\_SWITCH\_FILE** parameter in accuracy comparison is that **FUSION\_SWITCH\_FILE** can only disable the rules of graph fusion and UB fusion, and requires a separate JSON file to be configured, whereas this parameter applies to all rules and can specify fusion patterns directly through the parameter, without the need to configure a separate JSON file. If both parameters are configured and the same fusion pattern is configured, the configuration of the **OPTIMIZATION\_SWITCH** parameter takes precedence.

**Parameter values:** "Passname1:on;Passname2:off". Multiple key-value pairs can be concatenated, where the key is the Pass name and the value is **on** (enabled) or **off** (disabled). Case-sensitive pattern matching is not supported. Multiple configurations are separated by English semicolons. For configurable fusion patterns, see [Fusion Rule List](../../../../../user_guides/graph_dev/appendix/fusion_rule_list.md).

**Configuration example:**

```c++
{ge::ir_option::OPTIMIZATION_SWITCH, "Passname1:on;Passname2:off"}
```

**Applicable products:**

Supported on all chips.
