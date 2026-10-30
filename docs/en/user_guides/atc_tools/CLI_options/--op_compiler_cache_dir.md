# --op\_compiler\_cache\_dir

<!-- md-trans-meta sourceCommit=6910bdd915e3528be24078d698c2568ed03b113b translatedAt=2026-10-22T03:25:58.671Z pushedAt=2026-10-26T13:15:25.314Z -->

## Supported Products

Supported on all chips

## Function Description

Configures the path of the operator compilation disk cache.

## Related Parameters

To specify the path of the operator compilation disk cache, you need to use --op\_compiler\_cache\_dir together with the [--op\_compiler\_cache\_mode](--op_compiler_cache_mode.md) parameter.

## Parameter Values

**Parameter value:**
Path for storing the operator compilation disk cache.

**Parameter value format:**
The path supports uppercase and lowercase letters (a-z, A-Z), digits (0-9), underscores (\_), hyphens (-), periods (.), and Chinese characters.

**Parameter value constraints:**

- If the path specified by the [--op\_compiler\_cache\_dir](--op_compiler_cache_dir.md) parameter exists and is valid, a subdirectory kernel\_cache is automatically created under the specified path. If the specified path does not exist but is valid, the directory is automatically created first, and then a subdirectory kernel\_cache is automatically created under that path.
- Do not store your own content in the **default cache directory**. Your own content will be deleted together with the default cache directory when the software package is installed or upgraded.
- The **non-default cache directory** specified by this parameter cannot be deleted (it will not be deleted when the software package is installed or upgraded).

**Parameter default value:** $HOME/atc\_data

## Recommended Configuration and Benefits

None.

## Example

```bash
atc --op_compiler_cache_dir=$HOME/atc_data --op_compiler_cache_mode=enable ...
```

## Usage Constraints

In addition to the [--op\_compiler\_cache\_dir](--op_compiler_cache_dir.md) parameter, the operator compilation disk cache path can also be configured through the environment variable ASCEND\_CACHE\_PATH. The priority of these methods is as follows:

Configuration parameter “[--op\_compiler\_cache\_dir](--op_compiler_cache_dir.md)”\> environment variable ASCEND\_CACHE\_PATH\> default storage path.

For details about the environment variable ASCEND\_CACHE\_PATH, see [Environment Variable Reference](https://gitcode.com/cann/docs/blob/9.2.0/docs/en/env-vars/README.md).
