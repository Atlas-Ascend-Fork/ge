# Operator and Graph Compilation

<!-- md-trans-meta sourceCommit=6693584152164a17682ed389feb8e51961ab9533 translatedAt=2026-10-16T11:55:58.842Z pushedAt=2026-10-28T07:40:22.216Z nl=2 -->

## EXCLUDE\_ENGINES

Configures graph compilation to exclude one or more acceleration engines. Multiple engines are separated by "|". The corresponding **options** parameter of this parameter is `ge.exec.exclude_engines`.

The NPU integrates multiple hardware accelerators (also called acceleration engines), such as AiCore/AiVec/AiCpu (arranged by priority). During graph compilation, an appropriate engine is selected for each operator according to priority. That is, when the same operator is supported by multiple engines, the engine with the higher priority is selected.

**EXCLUDE\_ENGINES** provides the capability to exclude an engine. For example, during a training process, to prevent the data preprocessing graph and the main training graph from competing for AiCore, you can configure the data preprocessing graph not to use the AiCore engine.

**Parameter values:**

- "AiCore": AI Core hardware acceleration engine
- "AiVec": Vector Core hardware acceleration engine
- "AiCpu": AI CPU hardware acceleration engine

**Configuration example:**

```c++
{ge::ir_option::EXCLUDE_ENGINES, "AiCore|AiVec"}
```

**Applicable products:**

Supported on all chips.

## OP\_COMPILER\_CACHE\_MODE

Configures the operator compilation disk cache mode. The actual **options** parameter corresponding to this parameter is `ge.op_compiler_cache_mode`.

**Parameter values:**

- **enable**: (default value) Enables the operator compilation cache. After it is enabled, repeated compilation of operators with the same compilation parameters and operator parameters can be avoided, thereby improving compilation speed.
- **force**: Enables the operator compilation cache. Unlike the **enable** mode, the **force** mode forcibly refreshes the cache, that is, it first deletes the existing cache and then recompiles and adds it to the cache. For example, when the user's Python changes, dependency libraries change, or the repository changes after operator tuning, you need to first set the mode to **force** to clear the existing cache, and then change it to **enable** mode to avoid forcibly refreshing the cache on every compilation.
- **disable**: Disables the operator compilation cache, and operators are recompiled.

**Configuration example:**

```c++
{ge::ir_option::OP_COMPILER_CACHE_MODE, "enable"}
```

**Usage notes:**

- To specify the operator compilation disk cache path by yourself, use it together with **OP\_COMPILER\_CACHE\_DIR**.
- When the operator compilation cache is enabled, you can set the disk space size of the cache folder in two ways: through the **configuration file** (after operator compilation is complete, an **op\_cache.ini** file is automatically generated in the path specified by the **OP\_COMPILER\_CACHE\_DIR** parameter) and through **environment variables**:

    1. Set it through the configuration file op\_cache.ini

        If the op\_cache.ini file does not exist, create it manually. Open the file and add the following information:

        ```ini
        #Configuration file format, which must be included. The automatically generated file includes the following information by default. When creating the file manually, enter it.
        [op_compiler_cache]
        #Limit the disk space size of the cache folder on a chip. The default value is 500, the value must be an integer, and the unit is MB.
        max_op_cache_size=500
        #Set the proportion of cache space to retain. Value range: [1,100], default value: 50, unit: percentage. For example, a value of 80 means that when the cache space is insufficient, cache files are deleted and 80% of the cache space is retained.
        remain_cache_size_ratio=50
        ```

        - The op\_cache.ini file takes effect only when both the max\_op\_cache\_size and remain\_cache\_size\_ratio parameter values are valid.
        - When the compilation cache file size exceeds the value set by “max\_op\_cache\_size” and the cache file has not been accessed for more than half an hour, the cache file is aged. (During operator compilation, the compilation is not interrupted because the compilation cache file size exceeds the set value. Therefore, when “max\_op\_cache\_size” is set too small, the actual compilation cache file size may exceed this set value.)
        - To disable the compilation cache aging function, set “max\_op\_cache\_size” to "-1". In this case, the access time is not updated when the operator cache is accessed, the operator compilation cache is not aged, and the disk space uses the default size of 500 MB.
        - If multiple users use the same cache path, it is recommended to use the configuration file for the setting. In this scenario, the op\_cache.ini file affects all users.

    2. Set it through environment variables

        In this scenario, developers can use the environment variable ASCEND\_MAX\_OP\_CACHE\_SIZE to limit the disk space size of the cache folder under a specific chip. When the compilation cache space reaches the value set by ASCEND\_MAX\_OP\_CACHE\_SIZE and the cache file has not been accessed for more than half an hour, the cache file is aged out. The environment variable ASCEND\_REMAIN\_CACHE\_SIZE\_RATIO can be used to set the proportion of cache space to be retained.

        The configuration example is as follows:

        ```bash
        # The default value of the ASCEND_MAX_OP_CACHE_SIZE environment variable is 500. The value must be an integer, in MB.
        export ASCEND_MAX_OP_CACHE_SIZE=500
        # The value range of the ASCEND_REMAIN_CACHE_SIZE_RATIO environment variable is [1,100], and the default value is 50, in percentage. For example, a value of 80 means that when the cache space is insufficient, cache files are deleted and 80% of the cache space is retained.
        export ASCEND_REMAIN_CACHE_SIZE_RATIO=50
        ```

        - Configuration through environment variables takes effect only for the current user.
        - To disable the compilation cache aging function, set the environment variable “**ASCEND\_MAX\_OP\_CACHE\_SIZE**” to "-1". In this case, the access time is not updated when the operator cache is accessed, the operator compilation cache is not aged out, and the disk space uses the default size of 500 MB.

    **If both the op\_cache.ini file and the environment variable are configured, the configuration items in the op\_cache.ini file are read first. If neither the op\_cache.ini file nor the environment variable is set, the system default values are read: the default disk space size is 500 MB, and 50% of the cache space is retained by default.**

- Because the force option deletes the existing cache first, it is not recommended to set it during parallel program compilation. Otherwise, other models may fail to compile because the cache content they use is cleared.
- It is recommended to set the compilation cache option to disable or force when the model is finally released.
- If the repository changes after operator tuning, set the option to force to refresh the cache. Otherwise, the new tuning repository cannot be applied, causing the tuning application to fail.
- Scenarios where the debug switch is enabled:
  - **OP\_DEBUG\_LEVEL** is configured with a non-zero value: the configuration of the **OP\_COMPILER\_CACHE\_MODE** parameter is ignored, the operator compilation cache feature is not enabled, and all operators are recompiled.
  - **OP\_DEBUG\_CONFIG** is configured with a non-empty value, and the **OP\_DEBUG\_LIST field is not configured**: the configuration of the **OP\_COMPILER\_CACHE\_MODE** parameter is ignored, the operator compilation cache feature is not enabled, and all operators are recompiled.
  - **OP\_DEBUG\_CONFIG** is configured with a non-empty value, and the **OP\_DEBUG\_LIST** field is configured in the configuration file:
    - For operators in the list, the configuration of the OP\_COMPILER\_CACHE\_MODE parameter is ignored and they are still recompiled.
    - For operators outside the list, if the OP\_COMPILER\_CACHE\_MODE parameter is configured as enable or force, the cache feature is enabled; if it is configured as disable, the cache feature is not enabled and they are still recompiled.

**Applicable products:**

Supported on all chips.

## OP\_COMPILER\_CACHE\_DIR

Configures the directory for the operator compilation disk cache. The options parameter actually corresponding to this parameter is `ge.op_compiler_cache_dir`.

**Parameter value format**: The path supports uppercase and lowercase letters (a-z, A-Z), digits (0-9), underscores (\_), hyphens (-), periods (.), and Chinese characters.

**Default value**: $HOME/atc\_data

**Configuration example:**

```c++
{ge::ir_option::OP_COMPILER_CACHE_MODE, "enable"},
{ge::ir_option::OP_COMPILER_CACHE_DIR, "/home/test/data/atc_data"}
```

**Constraints:**

- To specify a custom path for the operator compilation disk cache, this parameter must be used together with **OP\_COMPILER\_CACHE\_MODE**.
- If the path specified by the parameter exists and is valid, a subdirectory **kernel\_cache** is automatically created under the specified path. If the specified path does not exist but is valid, the directory is automatically created first, and then a subdirectory **kernel\_cache** is automatically created under that path.
- Do not store your own content in the **default cache directory**. Your own content will be deleted together with the default cache directory when the software package is installed or upgraded.
- The **non-default cache directory** specified by this parameter cannot be deleted (it will not be deleted when the software package is installed or upgraded).
- Operator compilation disk cache path. In addition to the method set by the **OP\_COMPILER\_CACHE\_DIR** parameter, you can also configure the environment variable **ASCEND\_CACHE\_PATH**. The priority of these methods is as follows: configuration parameter "OP\_COMPILER\_CACHE\_DIR" \> environment variable ASCEND\_CACHE\_PATH \> default storage path. For details about the environment variable **ASCEND\_CACHE\_PATH**, see [*Environment Variable Reference*](https://gitcode.com/cann/docs/blob/9.2.0/docs/en/env-vars/README.md).

**Applicable products:**

Supported on all chips.

## OPTIMIZATION\_SWITCH

During operator compilation, this parameter is the control switch for fusion rules (passes) and applies to all fusion rules. The actual options parameter corresponding to this parameter is `ge.optimizationSwitch`.

**Parameter values:** "Passname1:on;Passname2:off". Multiple key-value pairs can be concatenated, where the key is the pass name and the value is on (enabled) or off (disabled). Case-sensitive pattern matching is not supported. Multiple configurations are separated by semicolons. For configurable fusion rules, see [fusion rule list](../../../../../user_guides/graph_dev/appendix/fusion_rule_list.md).

**Configuration example:**

```c++
{ge::ir_option::OPTIMIZATION_SWITCH, "Passname1:on;Passname2:off"}
```

**Applicable products:**

Supported on all chips.
