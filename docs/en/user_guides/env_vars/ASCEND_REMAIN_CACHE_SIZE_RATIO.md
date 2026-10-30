# ASCEND\_REMAIN\_CACHE\_SIZE\_RATIO

<!-- md-trans-meta sourceCommit=fb605ff2f9146cf0c88492555f75ac4506c57d51 translatedAt=2026-10-20T12:43:37.790Z pushedAt=2026-10-28T11:27:23.725Z nl=2 -->

## Function Description

When the operator compilation cache function is enabled and the compilation cache space reaches ASCEND\_MAX\_OP\_CACHE\_SIZE, requiring old kernel files to be deleted, the system needs to retain a ratio of the cache space. The default value is **50**, in percentage.

## Configuration Example

```bash
export ASCEND_REMAIN_CACHE_SIZE_RATIO=50
```

## Usage Constraints

The system reads the configuration for the ratio of retained cache space in the following priority order:

The “op\_cache.ini” configuration file in the operator compilation disk cache directory \> ASCEND\_REMAIN\_CACHE\_SIZE\_RATIO environment variable \> default value

That is, if a developer configures both the op\_cache.ini file and the environment variable, the configuration items in the op\_cache.ini file are read first. If neither the op\_cache.ini file nor the environment variable is set, the system default value is used: 50% of the cache space is retained by default.

For details about the “op\_cache.ini” configuration file, see [op\_cache.ini File Description](#op_cacheini-file-description).

## op\_cache.ini File Description

After the operator compilation cache function is enabled, the op\_cache.ini file is automatically generated in the specified operator compilation disk cache directory. Developers can configure the disk cache space size through this configuration file. If the op\_cache.ini file does not exist, you can manually create it in this path.

The methods for specifying the operator compilation disk cache directory vary by scenario. For example:

- In the scenario of developing AI applications based on the acl APIs, you can set it through the compilation option ACL\_OP\_COMPILER\_CACHE\_DIR in the "aclCompileOpt" API.
- In the graph development scenario, you can set it through the configuration parameter “ge.op\_compiler\_cache\_dir”.
- In the ATC offline model conversion scenario, you can set it through the parameter “--op\_compiler\_cache\_dir”.
<!-- npu="950,A3,910b,910" id1 -->
- In the PyTorch framework scenario, you can set it through the environment variable “ACL\_OP\_COMPILER\_CACHE\_DIR”. For details about “ACL\_OP\_COMPILER\_CACHE\_DIR”, see *[TorchNPU Environment Variables](https://www.hiascend.com/document/detail/en/Pytorch/latest/apiref/ENV/docs/en/environment_variable_reference/env_variable_list.md)*.
<!-- end id1 -->
<!-- npu="950,A3,910b,910,310p" id2 -->
- In the TensorFlow framework scenario, you can set it through the configuration parameter “op\_compiler\_cache\_dir”.

    For details about the TF Adapter configuration parameter “op\_compiler\_cache\_dir”, see:

  - *[TensorFlow 1.15 Model Migration](https://hiascend.com/document/redirect/tfcommunityguide)*, TF Adapter 1.x API Reference \> session configuration \> session configuration parameter description.
  - *[TensorFlow 2.6.5 Model Migration](https://hiascend.com/document/redirect/tfcommunityguide)* in the TF Adapter 2.x API Reference \> npu.global\_options \> Configuration Parameter Description.
<!-- end id2 -->

> [!NOTE]
>
>The scenarios listed above are only examples. If they do not cover your scenario, see the user guide for the corresponding scenario.

In the “op\_cache.ini” file, add the following information:

```ini
#Configuration file format, which must be included. The automatically generated file includes the following information by default. When you create the file manually, you need to fill in this information.
[op_compiler_cache]
#Limit the disk space occupied by cache files on a specific AI processor. The value is an integer in MB.
max_op_cache_size=500
#When the disk space is insufficient, set the ratio of cache files to retain. Value range: [1,100], in percentage. For example, 80 indicates that when the disk space is insufficient, 80% of the files in the cache space are retained and the rest are deleted.
remain_cache_size_ratio=80
```

## Applicable Products

All chips supported
