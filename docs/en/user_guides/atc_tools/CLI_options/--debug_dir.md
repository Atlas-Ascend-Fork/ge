# --debug\_dir

<!-- md-trans-meta sourceCommit=6910bdd915e3528be24078d698c2568ed03b113b translatedAt=2026-10-22T02:53:13.605Z pushedAt=2026-10-23T08:58:09.880Z -->

## Product Support

All chips are supported.

## Function Description

Configures the path of the debug-related process files generated during operator compilation in model conversion.

The process files include but are not limited to operator .o (operator binary file), .json (operator description file), and .cce files. The specific files generated depend on the value set by the [--op\_debug\_level](--op_debug_level.md) parameter.

## Related Parameters

To specify the storage path of the process files generated during operator compilation, use the --debug\_dir parameter together with the [--op\_debug\_level](--op_debug_level.md) parameter (with a non-zero value).

## Parameter Values

**Parameter value:**
Path of the debug-related files generated during operator compilation.

**Parameter value format:**
The path supports uppercase and lowercase letters (a-z, A-Z), digits (0-9), underscores (\_), hyphens (-), periods (.), and Chinese characters.

**Parameter value constraints:**
If this parameter is used, create the directory to be specified by this parameter before running the atc command, and ensure that the directory has read and write permissions.

**Default value:**
The process files generated during operator compilation are stored in the ./kernel\_meta folder in the current directory where the atc command is executed.

## Recommended Configuration and Benefits

None.

## Example

For example, if the created directory is named debug\_info, run the following command:

```bash
atc --debug_dir=$HOME/module/out/debug_info --op_debug_level=1 ...
```

## Usage Constraints

The storage path of the debug files generated during operator compilation can be configured not only through the [--debug\_dir](--debug_dir.md) parameter but also through the environment variable ASCEND\_WORK\_PATH. The priorities of these methods are as follows: the configuration parameter “[--debug\_dir](--debug_dir.md)” \> the environment variable ASCEND\_WORK\_PATH \> the default storage path.

For details about the environment variable ASCEND\_WORK\_PATH, see [*Environment Variable Reference*](https://gitcode.com/cann/docs/blob/9.2.0/docs/en/env-vars/README.md).
