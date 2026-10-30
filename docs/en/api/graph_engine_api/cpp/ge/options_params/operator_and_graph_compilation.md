# Operator Build and Graph Compilation

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-10-26T12:44:39.044Z pushedAt=2026-10-28T07:42:19.752Z nl=2 -->

## ge.aicoreNum

Used to configure the AI Core count for operator build.

**Parameter values:** "integer1|integer2", separated by "|":

<!-- npu="950,A3,910b" id1 -->
- **Scenario 1**: For the following products, integer1 represents the number of Cube Cores in the AI Core used for operator build, and integer2 represents the number of Vector Cores in the AI Core used for operator build. Both integer1 and integer2 must be greater than or equal to 0 and less than or equal to the maximum number of Cube Cores and Vector Cores contained in the AI processor:

    <!-- npu="950" id5 -->
    Ascend 950PR&950DT products
    <!-- end id5 -->

    <!-- npu="A3" id6 -->
    Atlas A3 products
    <!-- end id6 -->

    <!-- npu="910b" id7 -->
    Atlas A2 products
    <!-- end id7 -->
    <!-- end id1 -->

<!-- npu="910,310p" id2 -->
- **Scenario 2**: For the following products, only integer1 needs to be configured, in the format "integer1|". Configuring integer2 does not take effect. It represents the AI Core count used for operator build:

    <!-- npu="310p" id3 -->
    Atlas inference products
    <!-- end id3 -->

    <!-- npu="910" id4 -->
    Atlas training products
    <!-- end id4 -->
<!-- end id2 -->

**Usage constraints:**

<!-- npu="950,A3,910b" id8 -->
- For scenario 1 in the parameter value:

    The maximum number of Cube Cores and Vector Cores contained in different AI processors can be viewed in the `${INSTALL_DIR}/<arch>-linux/data/platform_config/xxx.ini` file. As shown below, the AI processor has 24 Cube Cores and 48 Vector Cores.

    ```ini
    [SoCInfo]
    # The parameter is configured with the default value, which is the maximum value.
    ai_core_cnt=24
    cube_core_cnt=24
    vector_core_cnt=48
    ```
    <!-- end id8 -->

<!-- npu="910,310p" id9 -->
- For scenario 2 in the parameter value:

    The maximum number of AI Cores contained in different AI processors can be viewed in the `${INSTALL_DIR}/<arch>-linux/data/platform_config/xxx.ini` file. As shown below, the AI processor has 10 AI Cores.

    ```ini
    [SoCInfo]
    # AI Core default value, which is the maximum value.
    ai_core_cnt=10
    vector_core_cnt=8
    ```
<!-- end id9 -->
- Configuring the value to 0 does not cause an error, but it is treated as unconfigured and does not produce a core control effect. For example, configuring "integer 1|0" is equivalent to configuring "integer 1|".
- If this parameter is configured while the operator compilation cache function is enabled (the ge.op\_compiler\_cache\_mode parameter is configured as "enable" or "force", and the default value is "enable"), this parameter takes effect only during the first compilation. If you want this parameter to take effect during non-first compilations, you need to clear the compilation disk cache.

Here, $\{INSTALL\_DIR\} must be replaced with the file storage path after CANN software installation. Taking root user installation as an example, the default file storage path after installation is /usr/local/Ascend/cann. <arch\> indicates the specific operating system architecture, and xxx must be selected based on the actual product.

<!-- npu="950,A3,910b" id10 -->
**Configuration example:**

- Configuration example for scenario 1:

    ```c++
    {"ge.aicoreNum", "24|48"};
    ```
<!-- end id10 -->

<!-- npu="910,310p" id11 -->
- Configuration example for scenario 2

    ```c++
    {"ge.aicoreNum", "10|"};
    or
    {"ge.aicoreNum", "10"};
    ```
<!-- end id11 -->

**Required/Optional**: Optional

**Effective level**: Global/session

**Understanding the relationship among AI Core, Cube Core, and Vector Core**:

To help you understand the relationship among AI Core, Cube Core, and Vector Core, the definition of Core is clarified first. A Core is a compute core that has an independent Scalar compute unit. The Scalar compute unit typically handles functions such as SIMD (Single Instruction Multiple Data) instruction issue for a compute core, so it is also commonly referred to as the scheduling unit within the core. The AI data processing core unit varies by product and is currently classified into the following types:

- When the AI data processing core unit is AI Core:
  - Within AI Core, Cube and Vector share one Scalar scheduling unit.
    <!-- npu="910" id12 -->
    For example, Atlas training products
    <!-- end id12 -->

    ![Diagram](../../../figures/logic_arch_diagram.png)

  - Within an AI Core, Cube and Vector each have their own Scalar scheduling unit, so they are also called Cube Core and Vector Core. In this case, one Cube Core and a group of Vector Cores are defined as one AI Core, and the number of AI Cores is usually calculated based on how many Cube Cores there are.
    <!-- npu="910b" id13 -->
    For example, Atlas A2 products.
    <!-- end id13 -->

- When the AI data processing core units are AI Core and a separate Vector Core: both AI Core and Vector Core have independent Scalar scheduling units.
    <!-- npu="310p" id14 -->
    For example, Atlas inference products.
    <!-- end id14 -->

    ![Diagram](../../../figures/logic_arch_diagram_1.png)

## ge.AllowMultiGraphParallelCompile

Controls the capability of compiling multiple graphs in parallel with multiple threads within the same session. If configured as "1", variable format conversion is disabled (see ge.exec.variable\_acc).

**Parameter values:**

- "0", the default value, indicates that one thread compiles multiple graphs within the same session.
- "1", indicates that users are allowed to compile multiple graphs concurrently with multiple threads within one session.

**Usage constraints:**

- If configured as "1", ge.exec.variable\_acc cannot be configured as "True" at the same time; otherwise, a validation error is reported.
- If configured as "1", an error is reported immediately if a resource operator causes other graphs to be recompiled.

**Configuration example:**

```c++
{"ge.AllowMultiGraphParallelCompile", "1"};
```

**Required/Optional**: Optional

**Effective level**: global/session

## ge.graph\_compiler\_cache\_dir

Graph compilation disk cache directory. It is used together with ge.graph\_key. The graph compilation disk cache function takes effect only when both ge.graph\_compiler\_cache\_dir and ge.graph\_key are configured with non-empty values.

The configured cache directory must exist; otherwise, compilation fails.

After the graph changes, the original cache files become invalid. You need to manually delete the cache files in the cache directory or modify ge.graph\_key, and then recompile to generate new cache files.

For other usage constraints and specific usage methods, see [graph compilation cache](../../../../../user_guides/graph_dev/more_features/graph_compile_cache.md).

**Required/Optional**: Optional

**Effective Level**: session

## ge.graph\_key

Unique graph identifier. The value can contain only uppercase and lowercase letters (A-Z, a-z), digits (0-9), underscores (\_), and hyphens (-), and must not exceed 128 characters in length.

**Required/Optional**: Optional

**Effective level**: graph

## ge.op\_compiler\_cache\_mode

Used to configure the operator build disk cache mode.

**Parameter values:**

- enable: (default value) Enables the operator compilation cache. After it is enabled, repeated builds of operators with the same build parameters and operator parameters can be avoided, thereby improving the build speed.
- force: Enables the operator compilation cache. Different from the enable mode, the force mode forcibly refreshes the cache, that is, it deletes the existing cache first, and then rebuilds and adds the result to the cache. For example, when the user's Python changes, the dependency library changes, or the knowledge base changes after operator tuning, you need to first set the mode to force to clear the existing cache, and then change it to enable, so as to avoid forcibly refreshing the cache each time compilation is performed.
- disable: Disables the operator compilation cache, and the operator is rebuilt.

**Usage constraints:**

- To specify the operator build disk cache path, use it together with ge.op\_compiler\_cache\_dir.
- Because the force option deletes the existing cache first, it is not recommended to set it during parallel program compilation. Otherwise, the cache content used by other models may be cleared, causing failures.
- It is recommended that the compilation cache option be set to disable or force when the model is finally released.
- If the knowledge base changes after operator tuning, set the mode to force to clear the cache first, and then set it to enable to recompile. Otherwise, the new tuning knowledge base cannot be applied, causing the tuning application to fail.
- In the scenario where the debug switch is enabled:
  - When ge.opDebugLevel is a non-zero value, the configuration of the ge.op\_compiler\_cache\_mode parameter is ignored, the operator compilation cache function is not enabled, and all operators are recompiled.
  - When op\_debug\_config is configured with a non-empty value and the **op\_debug\_list field is not configured**, the configuration of the ge.op\_compiler\_cache\_mode parameter is ignored, the operator compilation cache function is not enabled, and all operators are recompiled.
  - When op\_debug\_config is configured with a non-empty value and the **op\_debug\_list field is configured in the configuration file**:
    - For operators in the list, the configuration of the ge.op\_compiler\_cache\_mode parameter is ignored and they are recompiled.
    - For operators outside the list, if the ge.op\_compiler\_cache\_mode parameter is configured as enable or force, the cache function is enabled; if it is configured as disable, the cache function is not enabled and they are still recompiled.

- When the operator compilation cache function is enabled, the disk space size of the cache folder can be set in two ways: through the **configuration file** (after operator compilation is complete, the op\_cache.ini file is automatically generated in the path specified by the ge.op\_compiler\_cache\_dir parameter) or through **environment variables**:

    1. Set it through the op\_cache.ini configuration file.

        If the op\_cache.ini file does not exist, create it manually. Open the file and add the following information:

        ```ini
        #Configuration file format, which must be included. The automatically generated file includes the following information by default, and you need to enter it when creating the file manually.
        [op_compiler_cache]
        #Limit the disk space of the cache folder on a chip. The default value is 500, the value must be an integer, and the unit is MB.
        max_op_cache_size=500
        #Set the ratio of cache space to retain. Value range: [1,100]. The default value is 50, and the unit is percentage. For example, a value of 80 means that when the cache space is insufficient, cache files are deleted and 80% of the cache space is retained.
        remain_cache_size_ratio=50
        ```

        - The op\_cache.ini file takes effect only when both the max\_op\_cache\_size and remain\_cache\_size\_ratio parameter values are valid.
        - When the compilation cache file size exceeds the value set by “max\_op\_cache\_size” and the cache file has not been accessed for more than half an hour, the cache file ages. (During operator build, the build is not interrupted because the compilation cache file size exceeds the set value. Therefore, when “max\_op\_cache\_size” is set too small, the actual compilation cache file size may exceed the set value.)
        - To disable the compilation cache aging function, set “max\_op\_cache\_size” to "-1". In this case, the access time is not updated when the operator cache is accessed, the operator compilation cache does not age, and the disk space uses the default size of 500 MB.
        - If multiple users use the same cache path, you are advised to use a configuration file for the settings. In this scenario, the op\_cache.ini file affects all users.

    2. Set through environment variables

        In this scenario, you can use the environment variable ASCEND\_MAX\_OP\_CACHE\_SIZE to limit the disk space of the cache folder on a chip. When the compilation cache space reaches the value set by ASCEND\_MAX\_OP\_CACHE\_SIZE and the cache file has not been accessed for more than half an hour, the cache file expires. You can use the environment variable ASCEND\_REMAIN\_CACHE\_SIZE\_RATIO to set the ratio of cache space to retain.

        The configuration example is as follows:

        ```bash
        # The default value of the ASCEND_MAX_OP_CACHE_SIZE environment variable is 500. The value must be an integer, in MB.
        export ASCEND_MAX_OP_CACHE_SIZE=500
        # The value range of the ASCEND_REMAIN_CACHE_SIZE_RATIO environment variable is [1,100], and the default value is 50, in percentage. For example, a value of 80 means that when the cache space is insufficient, cache files are deleted and 80% of the cache space is retained.
        export ASCEND_REMAIN_CACHE_SIZE_RATIO=50
        ```

        - When configured through environment variables, the settings take effect only for the current user.
        - To disable the compilation cache aging function, set the “**ASCEND\_MAX\_OP\_CACHE\_SIZE**” environment variable to "-1". In this case, the access time is not updated when the operator cache is accessed, the operator compilation cache does not age, and the disk space uses the default size of 500 MB.

    **If both the op\_cache.ini file and the environment variables are configured, the configuration items in the op\_cache.ini file are read first. If neither the op\_cache.ini file nor the environment variables are set, the system default values are used: the default disk space size is 500 MB, and 50% of the cache space is retained by default.**

**Configuration example:**

```c++
{"ge.op_compiler_cache_mode", "enable"};
```

**Required/Optional**: Optional

**Effective level**: Global/session/graph

## ge.op\_compiler\_cache\_dir

Used to configure the directory for the operator build disk cache.

**Parameter value format**: The path supports uppercase and lowercase letters (a-z, A-Z), digits (0-9), underscores (\_), hyphens (-), periods (.), and Chinese characters.

**Default value**: **$HOME/atc\_data

- If the path specified by the parameter exists and is valid, a subdirectory kernel\_cache is automatically created under the specified path. If the specified path does not exist but is valid, the directory is automatically created first, and then a subdirectory kernel\_cache is automatically created under that path.
- Do not store other user content in the **default cache directory**. User content is deleted together with the default cache directory when the software package is installed or upgraded.
- A **non-default cache directory** specified by this parameter cannot be deleted (it is not deleted when the software package is installed or upgraded).
- In addition to the ge.op\_compiler\_cache\_dir parameter, the operator build disk cache path can also be configured through the environment variable ASCEND\_CACHE\_PATH. The priorities of these methods are as follows: configuration parameter "ge.op\_compiler\_cache\_dir" \> environment variable ASCEND\_CACHE\_PATH \> default storage path. For details about the environment variable ASCEND\_CACHE\_PATH, see [Environment Variable Reference](https://gitcode.com/cann/docs/blob/9.2.0/docs/en/env-vars/README.md).

**Required/Optional**: Optional

**Effective level**: Global/session/graph

## ge.optimizationSwitch

Control switch for fusion rules (Pass) during operator build.

The difference between this parameter and ge.fusionSwitchFile is that ge.fusionSwitchFile can only disable graph fusion and UB fusion rules and requires a separate JSON file, whereas this parameter applies to all rules and allows you to specify fusion rules directly through the parameter without a separate JSON file. If both parameters are configured and the same fusion rule is configured, the configuration of ge.optimizationSwitch takes precedence.

**Parameter values:** "Passname1:on;Passname2:off". You can concatenate multiple key-value pairs, where the key is the Pass name and the value is on (enabled) or off (disabled). Case-sensitive pattern matching is not supported. Separate multiple configurations with semicolons. For configurable fusion rules, see [Fusion Rule List](../../../../../user_guides/graph_dev/appendix/fusion_rule_list.md).

**Configuration example:**

```c++
{"ge.optimizationSwitch", "Passname1:on;Passname2:off"};
```

**Required/Optional**: Optional

**Effective level**: Global/session/graph

## ge.topoSortingMode

Different graph traversal modes that can be selected when compiling operators in graph mode. This parameter is mainly intended for online inference scenarios.

**Parameter values:**

- 0: BFS, Breadth First Search, breadth-first traversal strategy.
- 1: (default value) DFS, Depth First Search, depth-first traversal strategy.
- 2: RDFS, Reverse DFS, reverse depth-first traversal strategy.
- 3: StableRDFS, stable topological order strategy. For existing operators in the graph, their computation order is not changed; for newly added operators in the graph, the RDFS traversal strategy is used.

**Configuration example:**

```c++
{"ge.topoSortingMode", "1"};
```

**Required/Optional**: Optional

**Effective level**: Global/session/graph
