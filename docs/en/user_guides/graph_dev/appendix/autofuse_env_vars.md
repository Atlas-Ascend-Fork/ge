# AutoFuse Environment Variables

<!-- md-trans-meta sourceCommit=bd3de52c8a808742e9c01b524792105f46cad44c translatedAt=2026-10-19T12:29:20.165Z pushedAt=2026-10-20T10:30:46.590Z nl=2 -->

This document summarizes the environment variables and control items commonly used during the running and debugging of the AutoFuse automatic fusion feature on the **GE** path and the **Inductor** path.

## Common Environment Variables

The following environment variables are core feature control items of AutoFuse and apply to both the **GE** path and the **Inductor** path.

| Environment Variable | Applicable Scenario | Function |
| :------------------- | :------------------- | :----------------------------------------------------------- |
| `AUTOFUSE_FLAGS`     | **GE** path, **Inductor** path | Controls AutoFuse features. Multiple control items are separated by semicolons. |
| `AUTOFUSE_DFX_FLAGS` | **GE** path, **Inductor** path | Controls AutoFuse debugging, used for fusion graph dump, code generation debugging, and Auto Tiling debugging. Multiple control items are separated by semicolons. |

### AUTOFUSE_FLAGS Control Item

This control point is used to control the AutoFuse feature. The following is **the minimal configuration for enabling only the basic AutoFuse fusion capability:**

```bash
export AUTOFUSE_FLAGS="--enable_autofuse=true"
```

- For the **GE** path, the preceding configuration is used to enable the basic AutoFuse fusion capability.

- For the **Inductor** path, AutoFuse is enabled by configuring the `ascendc` backend through `torch.compile`, and `AUTOFUSE_FLAGS` is mainly used to configure extended features.

The following table lists all optional control items, which can be used in combination as needed:

| Control Item                                   | Applicable Scenario               | Function                                 |
| :--------------------------------------- | :--------------------- | :--------------------------------------------------- |
| [--enable_autofuse](#--enable_autofuse)                      | **GE** path                 | Controls whether the overall automatic fusion feature is enabled.                       |
| [--autofuse_enable_pass](#--autofuse_enable_pass)                 | **GE** path                 | Controls whether the specified extended fusion capability is enabled.                     |
| [--autofuse_disable_pass](#--autofuse_disable_pass)                | **GE** path                 | Controls whether the specified extended fusion capability is disabled.                     |
| [--autofuse_enable_pgo](#--autofuse_enable_pgo)                  | **GE** path, **Inductor** path | Enables PGO, which selects tiling with better performance through pre-deployment on-board sampling. |
| [--autofuse_enhance_precision_blacklist](#--autofuse_enhance_precision_blacklist) | **GE** path                 | Controls whether the specified AscIR operator type skips precision enhancement.              |
| [--experimental_enable_jit_executor_v2](#--experimental_enable_jit_executor_v2)  | **GE** path                 | Enables graph partitioning.                                       |
| [--max_fusion_size](#--max_fusion_size)                      | **GE** path                 | Sets the maximum number of nodes contained in a single fusion operator.                 |
| [--recomputation_threshold](#--recomputation_threshold)              | **GE** path                 | Sets the recomputation threshold for automatic fusion.                             |

Example:

```bash
export AUTOFUSE_FLAGS="--enable_autofuse=true;--autofuse_enable_pass=reduce,concat"
```

### AUTOFUSE_DFX_FLAGS Control Items

`AUTOFUSE_DFX_FLAGS` is used for AutoFuse compilation, Auto Tiling, and fusion result debugging.

| Control Item | Applicable Scenario | Function |
| :----------------------------------- | :-------------------- | :----------------------------------------- |
| [--autofuse_att_algorithm](#--autofuse_att_algorithm) | **GE** path, **Inductor** path | Selects the Auto Tiling solving algorithm. |
| [--att_accuracy_level](#--att_accuracy_level) | **GE** path, **Inductor** path | Controls the solving precision of the Auto Tiling algorithm. |
| [--att_enable_multicore_ub_tradeoff](#--att_enable_multicore_ub_tradeoff) | **GE** path, **Inductor** path | Controls whether the trade-off strategy between multi-core utilization and UB utilization is enabled. |
| [--att_ub_threshold](#--att_ub_threshold) | **GE** path, **Inductor** path | Sets the UB utilization threshold of Auto Tiling. |
| [--att_corenum_threshold](#--att_corenum_threshold) | **GE** path, **Inductor** path | Sets the multi-core utilization threshold of Auto Tiling. |
| [--att_profiling](#--att_profiling) | **GE** path, **Inductor** path | Controls whether Auto Tiling Profiling is enabled. |
| [--autofuse_pgo_algo](#--autofuse_pgo_algo) | **GE** path | Selects the PGO algorithm. |
| [--autofuse_pgo_step_max](#--autofuse_pgo_step_max) | **GE** path | Sets the step size of the PGO pruning algorithm. |
| [--autofuse_pgo_topn](#--autofuse_pgo_topn) | **GE** path | Sets the number of candidate solutions for static PGO. |
| [--codegen_compile_debug](#--codegen_compile_debug) | **GE** path, **Inductor** path | Controls whether to retain intermediate files generated during fusion operator compilation. |
| [--debug_dir](#--debug_dir) | **GE** path, **Inductor** path | Specifies the save path of AscGraph dump graphs during fusion. |
| [--disable_lifting](#--disable_lifting) | **GE** path | Controls whether to disable lifting. |
| [--skip_node_names_cfg](#--skip_node_names_cfg) | **GE** path | Sets the operator names or operator types to be skipped during fusion. |

Example:

```bash
export AUTOFUSE_DFX_FLAGS="--codegen_compile_debug=true;--debug_dir=/path/to/dump"
```

## Path-Specific Environment Variables

The following environment variables are path-specific and are used only for debugging on specific paths:

### Inductor Path-Specific Variables

The following environment variables are used for compilation or runtime debugging on the **Inductor** path and do not apply to the **GE** path:

| Environment Variable                | Description                                                         | Usage                                      |
| :----------------------------------- | :----------------------------------------------------------- | :-------------------------------------------- |
| `TORCH_COMPILE_DEBUG`                | Enables PyTorch compilation debugging information and saves intermediate compilation products to the `torch_compile_debug` directory in the current directory. Directories prefixed with `autofused_` usually indicate fused operator products generated by the AscendC backend. | `export TORCH_COMPILE_DEBUG=1`                |
| `TORCHINDUCTOR_FORCE_DISABLE_CACHES` | Disables the **Inductor** cache and forces recompilation on every execution. This configuration increases compilation and graph launch time and is intended for debugging only. | `export TORCHINDUCTOR_FORCE_DISABLE_CACHES=1` |
| `TORCHINDUCTOR_NPU_BACKEND`          | This environment variable is used to select the Ascend C backend.                             | `export TORCHINDUCTOR_NPU_BACKEND="ascendc"`  |
| `ASCEND_LAUNCH_BLOCKING`             | Implements synchronous execution of Ascend kernels, making it easier to locate the first kernel that reports an error. This configuration reduces execution performance and is recommended only for problem localization. | `export ASCEND_LAUNCH_BLOCKING=1`             |

For more environment variables, see [Debugging Environment Variables](https://gitcode.com/Ascend/torchair/blob/26.2.0/experimental/_inductor_npu_ext/docs/manuals.md#2-%E8%B0%83%E6%B5%8B%E7%9B%B8%E5%85%B3%E7%8E%AF%E5%A2%83%E5%8F%98%E9%87%8F).

### GE Path-Specific Variables

Currently there are no **GE** path-specific environment variables. All **GE** path-related control items are already included in the AutoFuse shared environment variables.

## Environment Variable Details

### AUTOFUSE\_FLAGS Environment Variable Control Points

#### --enable\_autofuse

Controls whether the overall automatic fusion feature is enabled.

**Value:**

- **true**: enabled
- **false**: (default value) disabled

If this item is not configured, the overall automatic fusion feature is disabled. When it is disabled, all other control points below become invalid regardless of whether they are configured.

**Configuration example:**

```text
--enable_autofuse=true
```

**Usage constraints:**

After this item is enabled, the automatic fusion capability between Elemwise operators and Broadcast operators is enabled.

#### --autofuse\_disable\_pass

Controls whether the extended fusion capability is disabled.

**Value:**

- **reduce**: disables the fusion capability of Reduce operators.
- **concat**: disables the fusion capability of Concat operators.

When configuring multiple values, separate them with commas. The default value is empty. The fusion capability of Reduce and Concat operators is disabled by default.

**Configuration example:**

```text
--autofuse_disable_pass=reduce,concat
```

**Usage constraints:**

It cannot be configured with the same value as **--autofuse\_enable\_pass**.

#### --autofuse\_enable\_pass

Controls whether the extended fusion capability is enabled.

**Value:**

- **reduce**: controls whether the Reduce fusion capability is enabled.
- **concat**: controls whether the Concat fusion capability is enabled.

When multiple values are configured, separate them with commas. The default value is empty, and the fusion capability of Reduce and Concat operators is disabled by default.

**Configuration example:**

```text
--autofuse_enable_pass=reduce,concat
```

**Usage constraints:**

It cannot be configured with the same value as **--autofuse\_disable\_pass**.

#### --autofuse\_enable\_pgo

Controls whether to enable profile-guided optimization (PGO).

PGO selects tiling with relatively better performance through pre-board sampling to improve model execution performance.

**Value:**

- **true**: enabled
- **false**: (default value) disabled

**Configuration example:**

```text
--autofuse_enable_pgo=true
```

**Usage constraints:**

Only static graph optimization is supported. When configured for the first time, it cannot be enabled simultaneously with other profiling features. There is no restriction for subsequent configurations.

#### --autofuse\_enhance\_precision\_blacklist

Controls whether precision improvement is skipped for local fusion nodes of automatic fusion.

Only when all AscIRs in a given AscGraph are configured in the blocklist will that AscGraph not promote precision.

**Value:**

- Array of AscIR types of the string type, with multiple types separated by commas. The default value is an empty string, which indicates that precision will be increased for all AscGraphs. The AscIR type in this parameter is the value of the corresponding node type in the dump graph. For example, if the type of a node in the dump graph is **ge:Add**, the actual configuration is **Add**.
- **all**, which indicates that all nodes are added to the blocklist, thereby disabling the precision improvement operation during automatic fusion and avoiding unnecessary precision improvement overhead.

**Configuration example:**

```text
--autofuse_enhance_precision_blacklist=Le,Where,Sub,Add,Sigmoid
```

**Usage constraints:**

- **Sum**, **Mean**, and **Prod** do not support low-precision types and precision must be increased for these types by default. Therefore, they cannot be configured in the blocklist for precision improvement. Even if this variable is set to **all**, precision still needs to be improved for these types.
- A better performance can be achieved by not improving precision, but it may cause precision issues. Therefore, after no precision improvement is configured, users must ensure that the precision meets their service requirements.
- For the method of dumping graphs, see the [DUMP\_GE\_GRAPH](../../../user_guides/env_vars/DUMP_GE_GRAPH.md) environment variable.

#### --experimental\_enable\_jit\_executor\_v2

Controls whether to enable graph partitioning.

**Value:**

- **true**: enabled
- **false**: (default value) disabled

Graph partitioning breaks the original graph at boundaries where symbols cannot be derived, dividing it into *N* graphs. The output of an upstream graph is used as the input hint of a downstream graph to continue symbol derivation and execution.

**Configuration example:**

```text
--experimental_enable_jit_executor_v2=true
```

**Usage constraints:**

Graph partitioning is not supported in the following scenarios:

- Dynamic profile scenario.
- The graph contains resource operators (the input or the type of the input is **DT\_RESOURCE**, such as **TensorArrayWrite**).
- The graph contains V1 control operators (such as **Switch**, **StreamSwitch**, **Merge**, **StreamMerge**, **Enter**, **Exit**, **LoopCond**, and **NextIteration**).
- The data preprocessing offload is enabled.
- The AOE optimization is enabled.

#### --max\_fusion\_size

Configures the maximum number of nodes that can be fused.

**Value:**
[0, Maximum value of the uint64_t type]. The value **0** indicates no fusion.

**Configuration example:**

```text
--max_fusion_size=64
```

The preceding example indicates that a maximum of 64 Ascir nodes can be fused.

**Usage constraints:**
None

#### --recomputation\_threshold

Controls the automatic fusion recomputation threshold.

This parameter is used to set the reference threshold of a single-output operator. When the number of references to a single-output node exceeds the threshold, the value is truncated at the current node during the first fusion.

**Value:**
An integer between 0 and 255. The default value is **1**.

**Configuration example:**

```text
--recomputation_threshold=5
```

**Usage constraints:**

None.

### AUTOFUSE\_DFX\_FLAGS Environment Variable Control Point

#### --autofuse\_att\_algorithm

Controls the selection of the Auto Tiling algorithm.

**Value:**

- **HighPerf**: high-performance algorithm
- **AxesReorder**: (default value) axis reordering algorithm

**Configuration example:**

```text
--autofuse_att_algorithm=HighPerf
```

**Usage constraints:**

- The axis reordering algorithm is the default algorithm. The high-performance algorithm is experimental and may not necessarily achieve better operator execution performance. It has a high performance only in theory.
- When an invalid value is configured, the default value will be restored.

#### --att\_accuracy\_level

Controls the solving precision of the Auto Tiling algorithm. In theory, the higher the precision, the better the kernel performance.

**Value:**

- **1**: (default value) high-precision solution
- **0**: low-precision solution

The default value is the high-precision solution, which guarantees that the tiling time is not excessively long.

**Configuration example:**

```text
--att_accuracy_level=1
```

**Usage constraints:**

1. The high-precision solution may produce a better tiling solution, but requires a longer tiling execution time. The low-precision solution is the opposite.
2. When an invalid value is configured, the default value will be restored.

#### --att\_enable\_multicore\_ub\_tradeoff

Controls whether the **att\_corenum\_threshold** and **att\_ub\_threshold** features are enabled.

**Value:**

- **true**: enabled
- **false**: (default value) disabled

**Configuration example:**

```text
--att_enable_multicore_ub_tradeoff=true
```

**Usage constraints:**
If an invalid value is configured, the default value will be restored.

#### --att\_ub\_threshold

Controls the tiling strategy of Auto Tiling to ensure that the tiling solution is combined with the operator implementation and that the UB occupancy rate is not lower than the value set by this control point. If the UB occupancy rate does not meet the specified threshold, it is set to the maximum UB occupancy rate that can be solved. This control point can be used to locate performance issues.

**Value:**
An integer between 0 and 100. The default value is **20**.

**Configuration example:**

```text
--att_ub_threshold=20
```

**Usage constraints:**

1. This parameter needs to be used together with [--att\_enable\_multicore\_ub\_tradeoff](#--att_enable_multicore_ub_tradeoff). Only when **--att\_enable\_multicore\_ub\_tradeoff** is enabled does the **--att\_ub\_threshold** control point take effect.
2. If an invalid value is configured, the default value will be restored.

#### --att\_corenum\_threshold

Controls the tiling strategy of Auto Tiling to ensure that the tiling solution is combined with the operator implementation, and that the multi-core utilization is not lower than the value set by this control point. If the multi-core utilization does not meet the specified threshold, the maximum achievable multi-core occupancy is used. This control point can be used to locate performance issues.

**Value:** An integer between 0 and 100. The default value is **40**.

When [--att\_enable\_multicore\_ub\_tradeoff](#--att_enable_multicore_ub_tradeoff) is enabled, the default value is **40**. Otherwise, this strategy is not set.

**Configuration example:**

```text
--att_corenum_threshold=40
```

**Usage constraints:**

1. This parameter needs to be used together with **--att\_enable\_multicore\_ub\_tradeoff**. Only when **--att\_enable\_multicore\_ub\_tradeoff** is enabled does the **--att\_corenum\_threshold** control point take effect.
2. If an invalid value is configured, the default value will be restored.

#### --att\_profiling

Controls whether profiling of Auto Tiling is enabled.

**Value:**

- **true**: Enables the profiling feature.
- **false**: (Default value) Disables the profiling feature.

**Configuration example:**

```text
--att_profiling=true
```

**Usage constraints:**

1. This control point is used only to locate execution time issues of the Auto Tiling module.
2. If an invalid value is configured, the default value will be restored.

#### --autofuse\_pgo\_algo

Controls the PGO algorithm. Tiling is computed differently using different algorithms.

**Value:**

- **core\_select**: (default value) uses the core-control algorithm.
- **pruning**: uses the pruning algorithm.

The default value is the core-control algorithm, which ensures that the tiling time is not excessively long. The pruning algorithm may theoretically find a better tiling solution, but it requires a much longer tiling time than the core-control algorithm.

**Configuration example:**

```text
--autofuse_enable_pgo=true --autofuse_pgo_algo=core_select
```

**Usage constraints:**

1. This parameter needs to be used together with **--autofuse\_enable\_pgo**. The **--autofuse\_pgo\_algo** control point takes effect only when **--autofuse\_enable\_pgo** is enabled.
2. If an invalid value is configured, the default value will be restored.

#### --autofuse\_pgo\_step\_max

Controls the step size of the PGO pruning algorithm.

**Value:**
Any power of 2 between 2 and 1024. The default value is **16**.

**Configuration example:**

```text
--autofuse_enable_pgo=true --autofuse_pgo_algo=pruning --autofuse_pgo_step_max=16
```

**Usage constraints:**

1. This parameter needs to be used together with **--autofuse\_pgo\_algo**. The **--autofuse\_pgo\_step\_max** control point takes effect only when **--autofuse\_pgo\_algo=pruning** is enabled and effective.

    When the step value is small, a better tiling solution may be obtained, but a longer tiling execution time is required. The opposite is true when the step value is large.

2. If an invalid value is configured, the default value will be restored.

#### --autofuse\_pgo\_topn

Sets the number of solutions used in static PGO.

**Value:**

- **0**: Selects all solutions for static optimization.
- Any positive integer: Indicates the number of selectable solutions. The default value is **5**.

**Configuration example:**

```text
--autofuse_enable_pgo=true --autofuse_pgo_topn=5
```

**Usage constraints:**

1. This parameter needs to be used together with **--autofuse\_enable\_pgo**. The **--autofuse\_pgo\_topn** control point takes effect only when **--autofuse\_enable\_pgo** is enabled.
2. If an invalid value is configured, the default value will be restored.

#### --codegen\_compile\_debug

Controls whether to retain the intermediate process files generated during fusion operator generation.

**Value:**

- **true**: retains the files.
- **false**: (defaults to false) does not retain the files.

**Configuration example:**

```text
--codegen_compile_debug=true
```

If this variable is set to **true**, after the files are retained, a **kernel\_meta\_\*** folder is generated in the script execution directory. The folder contains the generated kernel source code, tiling source code, CMake project, and compilation results. In addition, AscGraph dump graphs of each process in the fusion operator Schedule are generated under the **\{debug\_dir\}/autofuse\_compile\_debug/** directory.

**Usage constraints:**
None

#### --debug\_dir

Specifies the path used to save AscGraph dump graphs during fusion. The directory can contain letters, digits, underscores (\_), hyphens (-), and periods (.). The user must have read, write, and execute permissions on the directory. If this option is not specified, the files are saved to the current directory where the script is executed.

**Configuration example:**

```text
--codegen_compile_debug=true;--debug_dir=/path/to/dump
```

**Usage constraints:**
You must first configure **--codegen\_compile\_debug=true;** to enable the automatic fusion debug switch.

#### --disable\_lifting

Controls whether the lifting feature is disabled.

Automatic fusion fuses operators into a new operator named AscBackend, while lifting rolls back AscBackend operators that fail to meet the conditions to the original operators.

**Value:**

- **true**: disables the lifting feature.
- **false**: (default value) enables the lifting feature.

**Configuration example:**

```text
--disable_lifting=true
```

**Usage constraints:**

This control point is used only to locate analysis issues in the AscBackend rollback structure. Enabling this control point may cause precision exceptions in the ApplyAdamD operator.

#### --skip\_node\_names\_cfg

Set the operator name or operator type to skip fusion.

**Value:**
Directory and file name of the configuration file (.ini format) that contains the operator names or operator types. Each operator occupies one line. Both operator names and operator types can be set at the same time. The operator type must be the type of an operator defined based on Ascend IR.

**Configuration example:**

The following is an example of the .ini configuration file:

```ini
[ByNodeName]
op_name1
op_name2
[ByNodeType]
op_type1
op_type2
```

Parameter usage example:

```text
--skip_node_names_cfg=./skip_node.ini
```

**Usage constraints:**

When the content in the configuration file is invalid, this configuration item does not take effect.
