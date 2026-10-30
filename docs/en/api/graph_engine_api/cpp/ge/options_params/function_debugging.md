# Function Debugging

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-10-26T12:43:50.000Z pushedAt=2026-10-28T07:42:19.747Z nl=2 -->

## ge.debugDir

Configures the path for storing the process files generated during operator build for debugging. The process files include, but are not limited to, operator .o files (operator binary files), .json files (operator description files), and .cce files.

By default, the files are generated in the directory where the current training script is executed.

**Constraints:**

- To specify a custom path for storing the process files generated during operator build, use the ge.debugDir parameter together with the ge.opDebugLevel parameter. When ge.opDebugLevel is set to 0, the ge.debugDir parameter cannot be used.
- In addition to the ge.debugDir parameter, the storage path for the debug files generated during operator build can also be configured through the environment variable ASCEND\_WORK\_PATH. The priority of these methods is as follows: the configuration parameter “ge.debugDir”\> the environment variable ASCEND\_WORK\_PATH \> the default storage path. For details about the environment variable ASCEND\_WORK\_PATH, see [Environment Variable Reference](https://gitcode.com/cann/docs/blob/9.2.0/docs/en/env-vars/README.md).

**Required/Optional**: Optional

**Effective Level**: global/session/graph

## ge.exec.enable\_exception\_dump

Whether to dump data of abnormal operators.

- 0: (default value) Disables the abnormal operator data dump feature.
- 1: Enables normal exception dump (L1 exception dump), which dumps the input and output data, tensor description information (shape, dtype, format, and so on), and workspace information of abnormal operators.

    The dump data storage path priority is as follows: NPU\_COLLECT\_PATH\>ASCEND\_WORK\_PATH\> default path (the extra-info directory under the current script execution path)

- 2: Enables Lite exception dump (L0 exception dump), which dumps the input and output data, workspace information, and Tiling information of abnormal operators.

    The dump data storage path priority is as follows: ASCEND\_WORK\_PATH\> default path (the /extra-info/data-dump/<device\_id\> directory under the current script execution path)

> [!NOTE]
>
>- If the environment variable NPU\_COLLECT\_PATH is configured, only normal exception dump information is collected regardless of the value of the configuration item “ge.exec.enable\_exception\_dump”, including the input data and output data of abnormal operators, and the dump data is stored in the directory specified by the environment variable NPU\_COLLECT\_PATH.
>- L1 exception dump is normal exception dump, and L0 exception dump is Lite exception dump (lightweight exception dump). Both export information such as operator input/output and workspace data. However, compared with L0 exception dump, L1 exception dump provides more information. When L1 exception dump is enabled, information such as the dtype of each tensor is printed in the Host application log file (the plog log), and the operator name and kernel related to the operator are also printed.

For details about environment variables, see [Environment Variable Reference](https://gitcode.com/cann/docs/blob/9.2.0/docs/en/env-vars/README.md).

Configuration example:

```c++
std::map<ge::AscendString, ge::AscendString> ge_options = {"ge.exec.enable_exception_dump", "0"},
```

**Required/Optional**: Optional

**Effective Level**: Global

## ge.exportCompileStat

Configures whether to generate a result file fusion_result.json of operator fusion information (including graph fusion and UB fusion) during graph compilation.

This file records the fusion rules used during graph compilation. The ge.fusionSwitchFile parameter in precision comparison can disable specified fusion rules, and disabled fusion rules are not presented in the fusion\_result.json file. In the file:

- session\_and\_graph\_id\__xx\_xx_: indicates the thread and graph number to which the fusion result belongs.
- graph\_fusion: indicates graph fusion.
- ub\_fusion: indicates UB fusion

    <!-- npu="950" id1 -->
    **Ascend 950PR&950DT products do not support UB fusion, and this information is not generated**.
    <!-- end id1 -->

- match\_times: indicates the number of times a fusion rule is matched during graph compilation.
- effect\_times: indicates the number of times it actually takes effect.
- repository\_hit\_times: the number of hits in the optimized UB fusion knowledge base

    <!-- npu="950" id2 -->
    **Ascend 950PR&950DT products do not support UB fusion, so this information is not generated**.
    <!-- end id2 -->

**Parameter values:**

- 0: Do not generate the operator fusion information result file.
- 1: (Default value) Generate the operator fusion information result file when the program exits normally.
- 2: Generate the operator fusion information result file when graph compilation is complete. That is, if graph compilation is complete, the operator fusion information result file is generated even if the program is subsequently interrupted prematurely.

> [!NOTE]
>If the ASCEND\_WORK\_PATH environment variable is not set, the result file is generated in the current directory of the execution script by default. If the ASCEND\_WORK\_PATH environment variable is set, the save path is: $ASCEND\_WORK\_PATH/FE/$\{ process ID \}/fusion\_result.json. For details about the environment variable, see [Environment Variable Reference](https://gitcode.com/cann/docs/blob/9.2.0/docs/en/env-vars/README.md).

**Configuration example:**

```c++
{"ge.exportCompileStat", "1"};
```

**Required/Optional**: Optional

**Effective level**: Global

## ge.opDebugLevel

Feature switch for operator debugging. Valid values:

- 0: (Default value) Disables operator debugging. The operator build directory kernel\_meta is not generated in the current execution path.
- 1: Enables operator debugging. A kernel\_meta folder is generated in the current execution path, and within that folder, the \*.o (operator binary file), the \*.json file (operator description file), and the TBE instruction mapping file (the operator cce file \*.cce and the python-cce mapping file \*\_loc.json) are generated for subsequent analysis of AICore Error issues.

    <!-- npu="950" id3 -->
    The Ascend 950PR&950DT products do not generate the TBE instruction mapping file.
    <!-- end id3 -->

- 2: Enables operator debugging. A kernel\_meta folder is generated in the current execution path, and within that folder, the \*.o (operator binary file), the \*.json file (operator description file), and the TBE instruction mapping file (the operator cce file \*.cce and the python-cce mapping file \*\_loc.json) are generated for subsequent analysis of AICore Error issues. In addition, setting the value to 2 disables the compilation optimization switch and enables the ccec debugging function (the ccec compiler option is set to -O0-g).

    <!-- npu="950" id4 -->
    The Ascend 950PR&950DT products do not generate the TBE instruction mapping file.
    <!-- end id4 -->

- 3: Disables operator debugging. A kernel\_meta folder is generated in the current execution path, and within that folder, the \*.o (operator binary file) and the \*.json file (operator description file) are generated for reference when analyzing operator issues.
- 4: Disables operator debugging. A kernel_meta folder is generated in the current execution path, and within that folder, the *.o (operator binary file), the*.json file (operator description file), the TBE instruction mapping file (the operator cce file *.cce), and the UB fusion computation description file ({$kernel_name}_compute.json) are generated. These files can be used for issue reproduction and precision comparison when analyzing operator issues.

    <!-- npu="950" id5 -->
    The Ascend 950PR&950DT products do not generate the TBE instruction mapping file or the UB fusion computation description file.
    <!-- end id5 -->

> [!NOTE]
>
>- If ge.opDebugLevel is set to 0 and the op\_debug\_config parameter is also configured, the operator build directory kernel\_meta **is still generated** in the current execution path in this scenario.
>- During training execution, it is recommended to set this parameter to 0 or 3. If issue locating is required, select debug switch options 1 and 2, because enabling the debug function degrades network performance.
>- When this parameter is set to 2 (that is, the ccec compilation option is enabled), the size of the operator Kernel (the \*.o file) increases. In dynamic Shape scenarios, because the operator build traverses possible Shape scenarios, the operator Kernel file may become too large to be built. In this case, it is recommended not to configure the ccec compilation option. The following is an example of the error log reported when the operator Kernel file is too large to be built:
>
>   ```text
>   message:link error ld.lld: error: InputSection too large for range extension thunk ./kernel_meta_xxxxx.o
>    ```
>
>- When the debug feature switch is enabled, if the model contains the following general fusion operators, the \*.o, \*.json, and \*.cce files of the following operators are not generated in the operator build directory kernel\_meta.
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

**Required/Optional**: Optional

**Effective level**: Global/session/graph

## op\_debug\_config

Feature switch for Global Memory detection.

The value is a .cfg configuration file path. Multiple options in the configuration file are separated by commas:

- **oom**: During operator **execution**, detects whether Global Memory is accessed out of bounds.
  - When this option is configured, during operator build, the .o (operator binary) and .json (operator description) files are retained in the kernel\_meta folder generated by operator build in the current execution path.
  - When this option is used, the following detection logic is added during operator build. You can use the **dump\_cce** parameter to view the following code in the generated .cce file.

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

- **dump\_cce**: During operator build, the operator cce file \*.cce, as well as the .o (operator binary) and .json (operator description) files, are retained in the kernel\_meta folder generated by operator build in the current execution path.
- **dump\_loc**: During operator build, the python-cce mapping file \*\_loc.json, as well as the .o (operator binary) and .json (operator description) files, are retained in the kernel\_meta folder generated by operator build in the current execution path.
- **ccec\_O0**: During operator build, enables the ccec compiler option -O0. Configuring this option **does not** optimize the debug information, and is used for subsequent analysis of AI Core Error issues.
- **ccec\_g**: During operator build, enables the ccec compiler option -g. Configuring this option **does** optimize the debug information, and is used for subsequent analysis of AI Core Error issues.
- **check\_flag**: During operator **execution**, checks whether the pipeline synchronization signals inside the operator match.
  - When this option is configured, during operator build, the .o (operator binary) and .json (operator description) files are retained in the kernel\_meta folder generated by operator build in the current execution path.
  - After this option is used, the following detection logic is added during operator build. You can use the **dump\_cce** parameter to view the following code in the generated .cce file.

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

    During actual inference, if the pipeline synchronization signals inside the operator do not match, a timeout error is eventually reported **at the abnormal operator, and the program is terminated**. An example of the error information is as follows:

    ```text
    Aicore kernel execute failed, ..., fault kernel_name=operator name,...
    rtStreamSynchronizeWithTimeout execute failed....
    ```

**Configuration example:**

```c++
{"op_debug_config", "/root/test0.cfg"};
```

The test0.cfg file contains the following information:

```c++
op_debug_config = ccec_g,oom
```

**Required/Optional**: Optional

**Effective level**: Global

**Usage constraints:**

During operator build, if you do not want to build all AI Core operators but only certain AI Core operators, add the **op\_debug\_list** field to the test0.cfg configuration file mentioned above. During operator build, only the operators specified in this list are built, and they are built according to the options configured in op\_debug\_config. The requirements for the op\_debug\_list field are as follows:

- You can specify operator names or operator types.
- Separate operators with commas. For an operator type, configure it in the **OpType::TypeName** format. You can mix operator types and operator names.
- The operators to be built must be placed in the configuration file specified by the op\_debug\_config parameter.

The configuration example is as follows: Add the following information to the configuration file specified by the op\_debug\_config parameter (for example, test0.cfg):

```text
op_debug_config= ccec_g,oom
op_debug_list=GatherV2,opType::ReduceSum
```

During model build, the _GatherV2,ReduceSum_ operators are built according to the ccec\_g,oom options.

> [!NOTE]
>
>- When the ccec compilation option is enabled (that is, the ccec\_O0 or ccec\_g option), the size of the operator Kernel (the \*.o file) increases. In dynamic shape scenarios, because all possible scenarios are traversed during operator build, the operator Kernel file may eventually become too large to build. In this case, do not enable the ccec compilation option. The log for a build failure caused by an oversized operator kernel file is as follows:
>
>   ```text
>   message:link error ld.lld: error: InputSection too large for range extension thunk ./kernel_meta_xxxxx.o
>    ```
>
>- The ccec compiler options ccec\_O0 and oom cannot be enabled at the same time. Doing so may cause an AICore error. An example of the error information is as follows:
>
>   ```text
>   ...there is an aivec error exception, core id is 49, error code = 0x4 ...
>    ```
>
>- If the NPU\_COLLECT\_PATH environment variable is configured, the switch for detecting whether Global Memory is out of bounds (configuring oom in the configuration file specified by op\_debug\_config) is not supported. Otherwise, the compiled model file or operator kernel package will report an error when used.
>- When the compilation options oom, dump\_cce, and dump\_loc are configured, if the model contains the following general fusion operators, the \*.o, \*.json, and \*.cce files of these operators are not generated in the operator build directory kernel\_meta.
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
