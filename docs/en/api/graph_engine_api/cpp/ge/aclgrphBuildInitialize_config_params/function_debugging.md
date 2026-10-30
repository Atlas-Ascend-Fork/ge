# Function Debugging

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-10-16T11:33:22.404Z pushedAt=2026-10-28T07:40:22.195Z nl=2 -->

## DEBUG\_DIR

Configures the path for storing the debug-related process files generated during operator build. The process files include but are not limited to operator .o files (operator binary files), .json files (operator description files), and .cce files. The **options** parameter actually corresponding to this parameter is `ge.debugDir`.

By default, the files are generated in the current path.

**Constraints:**

- To specify the storage path for the process files generated during operator build, the **DEBUG\_DIR** parameter must be used together with the **OP\_DEBUG\_LEVEL** parameter. When **OP\_DEBUG\_LEVEL** is set to **0**, the **DEBUG\_DIR** parameter cannot be used.
- In addition to the **DEBUG\_DIR** parameter, the storage path for the debug files generated during operator build can also be configured through the environment variable **ASCEND\_WORK\_PATH**. The priority of these methods is as follows: the "DEBUG\_DIR" parameter > the ASCEND\_WORK\_PATH environment variable > the default storage path. For details about the **ASCEND\_WORK\_PATH** environment variable, see [Environment Variable Reference](https://gitcode.com/cann/docs/blob/9.2.0/docs/en/env-vars/README.md).

**Configuration example:**

```c++
{ge::ir_option::OP_DEBUG_LEVEL, "1"},
{ge::ir_option::DEBUG_DIR, "/home/test/module/out_debug_info"}
```

**Applicable products:**

Supported on all chips.

## OP\_DEBUG\_LEVEL

Feature switch for operator build debug. The actual **options** parameter corresponding to this parameter is `ge.opDebugLevel`.

To specify the storage path of the operator build process files, use the **DEBUG\_DIR** parameter. When **OP\_DEBUG\_LEVEL** is set to **0**, the **DEBUG\_DIR** parameter does not take effect.

**Parameter values:**

- **0**: (Default value) Disables the operator debug function and **does not generate** the operator build directory **kernel\_meta** in the current execution path.
- **1**: Enables the operator debug function, generates the **kernel\_meta** folder in the current execution path, and generates the *.o (operator binary file),*.json (operator description file), and TBE instruction mapping files (operator cce file *.cce and python-cce mapping file*_loc.json) in the folder for subsequent analysis of AICore Error issues.

    <!-- npu="950" id1 -->
    Ascend 950PR&950DT products do not generate the TBE instruction mapping file.
    <!-- end id1 -->

- **2**: Enables the operator debug function, generates the **kernel\_meta** folder in the current execution path, and generates the \*.o file (operator binary file), the \*.json file (operator description file), and the TBE instruction mapping file (operator cce file \*.cce and python-cce mapping file \*\_loc.json) in the folder, for subsequent analysis of AICore Error issues. Setting the value to **2** also disables the compilation optimization switch and enables the ccec debug function (the ccec compiler option is set to **-O0-g**).

    <!-- npu="950" id2 -->
    Ascend 950PR&950DT products do not generate the TBE instruction mapping file.
    <!-- end id2 -->

- 3: Disables the operator debug function, generates the **kernel\_meta** folder in the current execution path, and generates the \*.o file (operator binary file) and the \*.json file (operator description file) in the folder, which can be referenced when analyzing operator issues.
- 4: The operator debug function is disabled. A **kernel\_meta** folder is generated in the current execution path, and in this folder, the \*.o file (operator binary file), the \*.json file (operator description file), the TBE instruction mapping file (operator cce file \*.cce), and the UB fusion computation description file (\{$kernel\_name\}\_compute.json) are generated. These can be used for issue reproduction and precision comparison when analyzing operator issues.

    <!-- npu="950" id5 -->
    Ascend 950PR&950DT products do not generate the TBE instruction mapping file or the UB fusion computation description file.
    <!-- end id5 -->

> [!NOTE]
>
>- If **OP\_DEBUG\_LEVEL** is set to **0** and the **OP\_DEBUG\_CONFIG** parameter is also configured, the operator build directory **kernel\_meta** is retained in the current execution path in this scenario.
>- If **OP\_DEBUG\_LEVEL** is set to **0** and the **NPU\_COLLECT\_PATH** environment variable is also set, the build directory **kernel\_meta** is always retained. If the **ASCEND\_WORK\_PATH** environment variable is set, the directory is retained in the path specified by this environment variable; if the **ASCEND\_WORK\_PATH** environment variable is not set, it is retained in the current execution path.
>- During training execution, it is recommended to set the value to 0 or 3. If issue locating is required, select debug switch options 1 and 2, because enabling the debug function degrades network performance.
>- When the value is set to **2** (that is, the ccec compilation option is enabled), the size of the operator Kernel (the \*.o file) increases. In dynamic Shape scenarios, because the operator build traverses possible Shape scenarios, the operator Kernel file may become too large to compile. In this scenario, it is recommended not to configure the ccec compilation option. An example of the error log for a compilation failure caused by an oversized operator Kernel file is as follows:
>
>   ```text
>   message:link error ld.lld: error: InputSection too large for range extension thunk ./kernel_meta_xxxxx.o
>    ```
>
>- When the debug feature switch is enabled, if the model contains the following general fusion operators, the \*.o, \*.json, and \*.cce files of these operators are not generated in the operator build directory **kernel\_meta**.
>
>   MatMulAllReduce
>
>   MatMulAllReduceAddRmsNorm
>
>   AllGatherMatMul
>
>   MatMulReduceScatter
>
>   AlltoAllAllGatherBatchMatMul
>
>   BatchMatMulReduceScatterAlltoAll

**Configuration example:**

```c++
{ge::ir_option::OP_DEBUG_LEVEL, "1"}
```

**Applicable products:**

Supported on all chips.

## OP\_DEBUG\_CONFIG

Feature switch for Global Memory detection. The actual **options** parameter corresponding to this parameter is `op_debug_config`.

**Parameter values:**

The value is the path to a .cfg configuration file. Multiple options in the configuration file are separated by commas:

- **oom**: During operator **execution**, detects whether Global Memory is out of bounds.
  - When this option is configured, during operator build, the .o (operator binary file) and .json (operator description file) files are retained in the **kernel\_meta** folder generated by operator build in the current execution path.
  - After this option is used, the following detection logic is added during operator build. Users can use the **dump\_cce** parameter to view the following code in the generated .cce file.

    ```cce
    inline __aicore__ void  CheckInvalidAccessOfDDR(xxx) {
        if (access_offset < 0 || access_offset + access_extent > ddr_size) {
            if (read_or_write == 1) {
                trap(0X5A5A0001);
            } else {
                trap(0X5A5A0002);
            }
        }
    }
    ```

- **dump\_cce**: During operator build, the operator cce file \*.cce, as well as the .o (operator binary file) and .json (operator description file) files, are retained in the **kernel\_meta** folder generated by operator build in the current execution path.
- **dump\_loc**: During operator build, the python-cce mapping file \*\_loc.json, as well as the .o (operator binary file) and .json (operator description file) files, are retained in the **kernel\_meta** folder generated by operator build in the current execution path.
- **ccec\_O0**: During operator build, enables the ccec compiler option -O0. Configuring this option **does not** perform optimization on debug information, and is used for subsequent analysis of AI Core Error issues.
- **ccec\_g**: During operator build, enables the ccec compiler option **-g**. Configuring this option **does** optimize the debug information, which is used for subsequent analysis of AI Core Error issues.
- **check\_flag**: During operator **execution**, detects whether the internal pipeline synchronization signals of the operator match.
  - When this option is configured, during operator build, the .o (operator binary file) and .json (operator description file) files are retained in the **kernel\_meta** folder generated by operator build in the current execution path.
  - After this option is used, the following detection logic is added during the operator build process. Users can use the **dump\_cce** parameter to view the following code in the generated .cce file.

    ```cce
    set_flag(PIPE_MTE3, PIPE_MTE2, EVENT_ID0);
    set_flag(PIPE_MTE3, PIPE_MTE2, EVENT_ID1);
    set_flag(PIPE_MTE3, PIPE_MTE2, EVENT_ID2);
    set_flag(PIPE_MTE3, PIPE_MTE2, EVENT_ID3);
    ....
    pipe_barrier(PIPE_MTE3);
    pipe_barrier(PIPE_MTE2);
    pipe_barrier(PIPE_M);
    pipe_barrier(PIPE_V);
    pipe_barrier(PIPE_MTE1);
    pipe_barrier(PIPE_ALL);
    wait_flag(PIPE_MTE3, PIPE_MTE2, EVENT_ID0);
    wait_flag(PIPE_MTE3, PIPE_MTE2, EVENT_ID1);
    wait_flag(PIPE_MTE3, PIPE_MTE2, EVENT_ID2);
    wait_flag(PIPE_MTE3, PIPE_MTE2, EVENT_ID3);
    ...
    ```

    During actual inference execution, if the internal pipeline synchronization signals of the operator do not match, a timeout error is eventually reported **at the problematic operator, and the program is terminated**. An example of the error message is:

    ```text
    Aicore kernel execute failed, ..., fault kernel_name=operator name,...
    rtStreamSynchronizeWithTimeout execute failed....
    ```

**Configuration example:**

```c++
{ge::ir_option::OP_DEBUG_CONFIG, "/root/test0.cfg"}
```

The information in the test0.cfg file is as follows:

```text
op_debug_config=ccec_g,oom
```

**Usage constraints:**

During operator build, if users do not want to build all AI Core operators but instead specify certain AI Core operators for building, they need to add the **OP\_DEBUG\_LIST** field to the **test0.cfg** configuration file mentioned above. During operator build, only the operators specified in this list are built, and they are built according to the options configured by **OP\_DEBUG\_CONFIG**. The requirements for the **OP\_DEBUG\_LIST** field are as follows:

- Supports specifying an operator name or an operator type.
- Operators are separated by English commas. If an operator type is specified, configure it in the **OpType::TypeName** format. Mixed configuration of operator types and operator names is supported.
- Operators to be compiled must be placed in the configuration file specified by the **OP\_DEBUG\_CONFIG** parameter.

The following is a configuration example. Add the following information to the test0.cfg file:

```text
op_debug_config=ccec_g,oom
op_debug_list=GatherV2,opType::ReduceSum
```

During model compilation, the **_GatherV2,ReduceSum_** operators are compiled according to the **ccec\_g,oom** option.

> [!NOTE]
>
>- When the ccec compilation options are enabled (that is, the **ccec\_O0** and **ccec\_g** options), the size of the operator Kernel (the \*.o file) increases. In dynamic shape scenarios, because operator build traverses all possible scenarios, the operator Kernel file may eventually become too large to compile. In this case, it is recommended not to enable the ccec compilation option. The log displayed when the operator kernel file is too large to compile is as follows:
>
>   ```text
>   message:link error ld.lld: error: InputSection too large for range extension thunk ./kernel_meta_xxxxx.o
>    ```
>
>- The ccec compiler options **ccec\_O0** and **oom** cannot be enabled at the same time, which may cause an AICore Error. An example of the error message is as follows:
>
>   ```text
>   ...there is an aivec error exception, core id is 49, error code = 0x4 ...
>    ```
>
>- If the **NPU\_COLLECT\_PATH** environment variable is configured, enabling the switch for "detecting whether Global Memory is out of bounds" (configure **oom** in the configuration file specified by **OP\_DEBUG\_CONFIG**) is not supported. Otherwise, the compiled model file or operator kernel package will report an error when used.
>- When the compilation options **oom**, **dump\_cce**, and **dump\_loc** are configured, if the model contains the following general fusion operators, the \*.o, \*.json, and \*.cce files of these operators will not be generated in the operator build directory **kernel\_meta**.
>
>   MatMulAllReduce
>
>   MatMulAllReduceAddRmsNorm
>
>   AllGatherMatMul
>
>   MatMulReduceScatter
>
>   AlltoAllAllGatherBatchMatMul
>
>   BatchMatMulReduceScatterAlltoAll

**Applicable products:**

Supported on all chips.

## OPTION\_SCREEN\_PRINT\_MODE

Controls whether the graph compilation process prints to the screen. The corresponding **options** parameter of this parameter is `ge.screen_print_mode`.

**Parameter values:**

- **enable**: Prints to the screen. The default value is **enable**.
- **disable**: Does not print to the screen.

**Configuration example:**

```c++
{ge::ir_option::OPTION_SCREEN_PRINT_MODE, "disable"}
```

**Applicable products:**

Supported on all chips.

## OPTION\_EXPORT\_COMPILE\_STAT

Configures whether to generate a result file **fusion\_result.json** of operator fusion information (including graph fusion and UB fusion) during graph compilation. The **options** parameter actually corresponding to this parameter is `ge.exportCompileStat`.

This file is used to record the fusion rules used during graph compilation, while the **FUSION\_SWITCH\_FILE** parameter in precision comparison can disable specified fusion rules. Disabled fusion rules are not presented in the **fusion\_result.json** file. In the file:

- **session\_and\_graph\_id\__xx\_xx_**: Indicates the thread and graph number to which the fusion result belongs.
- **graph\_fusion**: Indicates graph fusion.
- **ub\_fusion**: Indicates UB fusion.

    <!-- npu="950" id3 -->
    **The Ascend 950PR&950DT products does not support UB fusion, so this information is not generated**.
    <!-- end id3 -->

- **match\_times**: Indicates the number of times a fusion rule is matched during graph compilation.
- **effect\_times**: Indicates the number of times it actually takes effect.
- **repository\_hit\_times**: Indicates the number of hits in the optimized UB fusion knowledge base.

    <!-- npu="950" id4 -->
    **The Ascend 950PR&950DT products does not support UB fusion, and this information is not generated**.
    <!-- end id4 -->

**Parameter values:**

- **0**: The operator fusion information result file is not generated.
- **1**: (Default value) The operator fusion information result file is generated when the program exits normally.
- **2**: The operator fusion information result file is generated when graph compilation is complete. That is, if graph compilation is complete, the operator fusion information result file is still generated even if the program is interrupted early.

> [!NOTE]
>
>- If the **ASCEND\_WORK\_PATH** environment variable is not set, the result file is generated in the current path of the execution script by default. If the **ASCEND\_WORK\_PATH** environment variable is set, the save path is `$ASCEND_WORK_PATH/FE/${process ID}/fusion_result.json`. For details about the environment variable, see [Environment Variable Reference](https://gitcode.com/cann/docs/blob/9.2.0/docs/en/env-vars/README.md).
>- Fusion rules disabled through the **FUSION\_SWITCH\_FILE** parameter are not presented in the **fusion\_result.json** file.

**Configuration example:**

```c++
{ge::ir_option::OPTION_EXPORT_COMPILE_STAT, "1"}
```

**Applicable products:**

Supported on all chips.
