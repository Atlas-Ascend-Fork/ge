# ASCEND\_MAX\_OP\_CACHE\_SIZE

<!-- md-trans-meta sourceCommit=fb605ff2f9146cf0c88492555f75ac4506c57d51 translatedAt=2026-11-13T09:52:35.729Z pushedAt=2026-11-16T06:11:08.631Z nl=2 -->

## Feature Description

When the operator build cache function is enabled, this environment variable can be used to limit the disk space size of the cache folder under a specific AI processor. The default value is 500, in MB.

- When the size of the build cache files exceeds the value set by this environment variable and the cache files have not been accessed for more than half an hour, the cache files are aged.
- To disable the build cache aging function, set the environment variable **ASCEND\_MAX\_OP\_CACHE\_SIZE** to **-1**. In this case, the access time is not updated when the operator cache is accessed, the operator build cache is not aged, and the default disk space of 500 MB is used.

> [!NOTE]
>During operator build, the build is not interrupted because the size of the build cache files exceeds the set value. Therefore, if the value of this environment variable is set too small, the actual size of the build cache files may exceed the set value.

## Configuration Example

```bash
export ASCEND_MAX_OP_CACHE_SIZE=500
```

## Constraints

The system reads the cache disk space size configuration in the following priority order:

The op\_cache.ini configuration file in the operator build disk cache directory \> ASCEND\_MAX\_OP\_CACHE\_SIZE environment variable \> default value.

That is, if the developer configures both the op\_cache.ini file and the environment variable, the configuration item in the op\_cache.ini file is read first. If neither the op\_cache.ini file nor the environment variable is set, the system default value is read: the default disk space size is 500 MB.

For details about the op\_cache.ini configuration file, see [op\_cache.ini File Description](#op_cacheini-file-description).

## op\_cache.ini File Description

After the operator compilation cache function is enabled, the op\_cache.ini file is automatically generated in the specified operator compilation disk cache directory. Developers can configure the disk cache space size through this configuration file. If the op\_cache.ini file does not exist, you can manually create it in this path.

The methods for specifying the operator compilation disk cache directory vary by scenario. For example:

- In the scenario of developing AI applications based on the ACL APIs, the directory can be set through the compilation option ACL\_OP\_COMPILER\_CACHE\_DIR in the [aclCompileOpt](../../api/graph_engine_api/c/acl/aclCompileOpt.md) API.
- In the graph development scenario, the directory can be set through the configuration parameter [ge.op\_compiler\_cache\_dir](../../api/graph_engine_api/cpp/ge/options_params/operator_and_graph_compilation.md#geop_compiler_cache_dir).
- In the ATC offline model conversion scenario, the directory can be set through the parameter [--op\_compiler\_cache\_dir](../atc_tools/CLI_options/--op_compiler_cache_dir.md).
<!-- npu="950,A3,910b,910" id1 -->
- In the PyTorch framework scenario, you can set it through the environment variable ACL\_OP\_COMPILER\_CACHE\_DIR. For details about ACL\_OP\_COMPILER\_CACHE\_DIR, see [TorchNPU Environment Variables](https://www.hiascend.com/document/detail/en/Pytorch/latest/apiref/ENV/docs/en/environment_variable_reference/env_variable_list.md).
<!-- end id1 -->
<!-- npu="950,A3,910b,910,310p" id2 -->
- In the TensorFlow framework scenario, you can set it through the configuration parameter op\_compiler\_cache\_dir.

    For details about the TF Adapter configuration parameter op\_compiler\_cache\_dir, see:

  - *[TensorFlow 1.15 Model Migration](https://hiascend.com/document/redirect/tfcommunityguide)*, TF Adapter 1.x API Reference \> session configuration \> session configuration parameter description.
  - *[TensorFlow 2.6.5 Model Migration](https://hiascend.com/document/redirect/tfcommunityguide)* in the TF Adapter 2.x API Reference \> npu.global\_options \> Configuration Parameter Description.

<!-- end id2 -->
> [!NOTE]
>The scenarios listed above are only examples. If they do not cover your scenario, see the user guide for the corresponding scenario.

In the op\_cache.ini file, add the following information:

```ini
#Configuration file format. This field is mandatory. The automatically generated file includes the following information by default. When the file is manually created, this field must be filled in.
[op_compiler_cache]
#Limit the disk space occupied by cache files of an AI processor. The value is an integer in MB.
max_op_cache_size=500
#When the disk space is insufficient, set the percentage of cache files to retain. Value range: [1,100], in percentage. For example, 80 indicates that when the disk space is insufficient, 80% of the files in the cache space are retained and the rest are deleted.
remain_cache_size_ratio=80
```

## Applicable Products

All chips supported
