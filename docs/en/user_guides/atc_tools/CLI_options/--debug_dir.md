# --debug\_dir

<!-- md-trans-meta sourceCommit=6693584152164a17682ed389feb8e51961ab9533 translatedAt=2026-11-16T13:44:46.711Z pushedAt=2026-11-17T08:11:31.367Z nl=2 -->

## Product Support

Supported on all chips.

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

## Constraints

In addition to the method of setting the [--debug\_dir](--debug_dir.md) parameter, you can also configure the environment variable ASCEND\_WORK\_PATH to specify the storage path of the debug files generated during operator build. The priority of these methods is as follows: configuration parameter "[--debug\_dir](--debug_dir.md)" > environment variable ASCEND\_WORK\_PATH > default storage path.

For details about the environment variable ASCEND\_WORK\_PATH, see [*Environment Variable Reference*](https://gitcode.com/cann/docs/blob/master/docs/en/env-vars/README.md).
