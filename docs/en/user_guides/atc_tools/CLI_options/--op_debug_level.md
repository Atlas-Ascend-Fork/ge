# --op\_debug\_level

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-22T03:27:47.910Z pushedAt=2026-10-29T08:18:28.869Z nl=2 -->

## Product Support

Supported on all chips.

## Function Description

Switch for the operator debug function.

## Related Parameters

To specify the path for storing the intermediate files of the operator build, --op\_debug\_level (set to a non-zero value) must be used together with [--debug\_dir](--debug_dir.md).

## Parameter Values

**Parameter values:**

- 0: (Default) Disables the operator debug function. The operator build directory kernel\_meta is **not generated** in the current execution path.
- 1: Enables the operator debug function. A kernel\_meta folder is generated in the current execution path, and ****\*.o (operator binary file), \*.json file (operator description file), and TBE instruction mapping file (operator cce file \*.cce and python-cce mapping file \*\_loc.json)** are generated in the folder for subsequent analysis of AICore Error issues.

    <!-- npu="950" id1 -->
    The Ascend 950PR&950DT  products do not generate TBE instruction mapping files.
    <!-- end id1 -->

- 2: Enables the operator debug function. A kernel\_meta folder is generated in the current execution path, and ****\*.o (operator binary file), \*.json file (operator description file), and TBE instruction mapping file (operator cce file \*.cce and python-cce mapping file \*\_loc.json)** are generated in the folder for subsequent analysis of AICore Error issues. In addition, setting the value to 2 disables the compilation optimization switch and enables the ccec debug function (the ccec compiler option is set to -O0-g).

    <!-- npu="950" id2 -->
    The Ascend 950PR&950DT  products do not generate TBE instruction mapping files.
    <!-- end id2 -->

- 3: Disables the operator debug function. A kernel\_meta folder is generated in the current execution path, and ****\*.o (operator binary file) and \*.json file (operator description file)** are generated in the folder for reference when analyzing operator issues.
- 4: Disables the operator debug function. A kernel\_meta folder is generated in the current execution path, and ****\*.o (operator binary file), \*.json file (operator description file), TBE instruction mapping file (operator cce file \*.cce), and UB fusion computation description file (\{$kernel\_name\}\_compute.json)** are generated in the folder for issue reproduction and precision comparison when analyzing operator issues.

    <!-- npu="950" id3 -->
    The Ascend 950PR&950DT  products do not generate TBE instruction mapping files or UB fusion computation description files.
    <!-- end id3 -->

**Parameter value constraints:**

- During model conversion, it is recommended to set the value to 0, 3, or 4. If you need to locate AICore Error issues, set the value to 1 or 2. After setting the value to 1 or 2, network performance degrades because debugging functions are added.
- If --op\_debug\_level is set to 0 and the [--op\_debug\_config](--op_debug_config.md) parameter is also configured, the operator build directory kernel\_meta **is retained** in the current path where the atc command is executed.
- If --op\_debug\_level is set to 0 and the NPU\_COLLECT\_PATH environment variable is also set, the build directory kernel\_meta is **always retained**. If the ASCEND\_WORK\_PATH environment variable is set, the directory is retained in the path specified by this environment variable. If the ASCEND\_WORK\_PATH environment variable is not set, the directory is retained in the current execution path.
- When the value is set to 2 (that is, the ccec build option is enabled), the size of the operator Kernel (the \*.o file) increases. In dynamic Shape scenarios, the operator build traverses possible Shape scenarios, which may cause the operator Kernel file to be too large to build. In this case, it is recommended not to configure the ccec build option.

    The following is an example of the error log reported when the operator Kernel file is too large to build:

    ```console
    message:link error ld.lld: error: InputSection too large for range extension thunk ./kernel_meta_xxxxx.o
    ```

- When the debug function switch is enabled, if the model contains the following general-purpose fusion operators, the \*.o, \*.json, and \*.cce files of these operators are not generated in the operator build directory kernel\_meta.

    MatMulAllReduce

    MatMulAllReduceAddRmsNorm

    AllGatherMatMul

    MatMulReduceScatter

    AlltoAllAllGatherBatchMatMul

    BatchMatMulReduceScatterAlltoAll

## Recommended Configuration and Benefits

None.

## Example

```bash
atc --op_debug_level=1 ...
```

## Constraints

- In addition to the method of setting the [--debug\_dir](--debug_dir.md) parameter, you can also configure the environment variable ASCEND\_WORK\_PATH to specify the storage path of the debug files generated during operator build. The priority of these methods is as follows: configuration parameter “[--debug\_dir](--debug_dir.md)”\> > environment variable ASCEND\_WORK\_PATH > \> default storage path.

    For details about ASCEND\_WORK\_PATH, see [*Environment Variable Reference*](https://gitcode.com/cann/docs/blob/9.2.0/docs/zh/env-vars/README.md).

- This parameter takes precedence over the value of **tbe\_debug\_level** in the operator build API (the build API of TBE DSL or the BuildCCE API of TBE TIK).
