# --op\_compiler\_cache\_mode

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-22T03:26:10.137Z pushedAt=2026-10-27T11:17:48.720Z -->

## Supported Products

Supported on all chips.

## Function Description

Configures the disk cache mode for operator build.

## Related Parameters

To specify the path of the operator build disk cache yourself, use the [--op\_compiler\_cache\_dir](--op_compiler_cache_dir.md) parameter.

## Parameter Values

**Parameter values:**

- enable: enables the operator build cache. After it is enabled, repeated builds of operators with the same build parameters and operator parameters can be avoided, thereby improving the build speed.
- force: enables the operator build cache. Different from the enable mode, the force mode forcibly refreshes the cache, that is, deletes the existing cache first, and then rebuilds and adds the cache. For example, when the user's Python changes, the dependency libraries change, or the knowledge base changes after operator tuning, you need to first set the mode to force to clear the existing cache, and then change it to enable to avoid forcibly refreshing the cache each time a build is performed.
- disable: (default value) disables the operator build cache, and operators are rebuilt.

**Parameter value constraints:**

1. Because the force option deletes the existing cache first, it is not recommended to set it during parallel program builds. Otherwise, the cache content used by other models may be cleared, causing failures.
2. It is recommended that you set the compilation cache option to disable or force when the model is finally released.
3. If the knowledge base changes after operator tuning, you need to set the mode to force to refresh the cache. Otherwise, the new tuning knowledge base cannot be applied, causing the tuning application to fail.
4. In the scenario where the debug switch is enabled:
    - [--op\_debug\_level](--op_debug_level.md) Configured with a non-zero value: the configuration of the --op\_compiler\_cache\_mode parameter is ignored, the operator compilation cache function is not enabled, and all operators are recompiled.
    - [--op\_debug\_config](--op_debug_config.md) Configured with a non-empty value, and the **op\_debug\_list field is not configured**: the configuration of the --op\_compiler\_cache\_mode parameter is ignored, the operator compilation cache function is not enabled, and all operators are recompiled.
    - [--op\_debug\_config](--op_debug_config.md) Configured with a non-empty value, and the **op\_debug\_list field is configured in the configuration file**:
        - Operators in the list ignore the configuration of the --op\_compiler\_cache\_mode parameter and continue to be recompiled.
        - For operators outside the list, if the --op\_compiler\_cache\_mode parameter is set to enable or force, the cache function is enabled; if it is set to disable, the cache function is not enabled and the operators are still recompiled.

## Recommended Configuration and Benefits

The recommended configuration is enable: after it is enabled, repeated operator builds with the same build parameters and operator parameters can be avoided, thereby improving the build speed.

## Example

```bash
atc --op_compiler_cache_mode=enable --op_compiler_cache_dir=$HOME/atc_data/kernel_cache --op_debug_level=0 ...
```

## Constraints

When the operator compilation cache function is enabled, you can set the disk space size of the cache folder in either of the following two ways: through the **configuration file** (after the ATC tool runs, the op\_cache.ini configuration file is automatically generated in the path specified by the [--op\_compiler\_cache\_dir](--op_compiler_cache_dir.md) parameter) or through **environment variables**:

1. Set through the configuration file op\_cache.ini

    If the op\_cache.ini file does not exist, create it manually. Open the file and add the following information:

    ```ini
    #Configuration file format, which must be included. The automatically generated file includes the following information by default. When creating the file manually, you need to enter it.
    [op_compiler_cache]
    #Limit the disk space size of the cache folder for a chip. The default value is 500, the value must be an integer, and the unit is MB.
    max_op_cache_size=500
    #Set the proportion of cache space to be retained. Value range: [1,100]. The default value is 50, and the unit is percentage. For example, a value of 80 means that when the cache space is insufficient, cache files are deleted and 80% of the cache space is retained.
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
    # The default value of the ASCEND_MAX_OP_CACHE_SIZE environment variable is 500. The value must be an integer, in MB
    export ASCEND_MAX_OP_CACHE_SIZE=500
    # The value range of the ASCEND_REMAIN_CACHE_SIZE_RATIO environment variable is [1,100], and the default value is 50, in percentage. For example, a value of 80 means that when the cache space is insufficient, cache files are deleted and 80% of the cache space is retained
    export ASCEND_REMAIN_CACHE_SIZE_RATIO=50
    ```

    - Configuration through environment variables takes effect only for the current user.
    - To disable the compilation cache aging function, set the environment variable “**ASCEND\_MAX\_OP\_CACHE\_SIZE**” to "-1". In this case, the access time is not updated when the operator cache is accessed, the operator compilation cache is not aged, and the default disk space size of 500 MB is used.

**If both the op\_cache.ini file and environment variables are configured, the configuration items in the op\_cache.ini file are read first. If neither the op\_cache.ini file nor the environment variables are set, the system default values are read: the default disk space size is 500 MB, and 50% of the cache space is retained by default.**
