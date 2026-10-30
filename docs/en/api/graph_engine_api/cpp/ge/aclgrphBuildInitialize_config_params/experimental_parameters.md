# Experimental Parameters

## DETERMINISTIC\_LEVEL

**This parameter is a debugging parameter and is not supported yet for use in production environments. It will be released as a formal feature in a later version.**

Deterministic computation level. The actual options parameter corresponding to this parameter is `ge.deterministicLevel`.

By default, the deterministic level is 0, meaning deterministic computation is not enabled, and the `DETERMINISTIC` option for whether to enable deterministic computation must also be 0. To enable deterministic computation, set the deterministic level to 1 and the `DETERMINISTIC` option to 1. To enable strong consistency computation, set the level to 2 and the `DETERMINISTIC` option to 1. To enable Batch consistency computation, set the level to 3 and the `DETERMINISTIC` option to 1.

When strong consistency computation is enabled (DETERMINISTIC\_LEVEL=2), the computation result is deterministic, and multiple executions produce the same output. In addition, the computation result is independent of the data location. For example, during matrix multiplication, the accumulation order of different rows may differ, which may cause slight differences in the computation results of the same data across different rows. However, with strong consistency computation enabled, as long as the input is the same, the computation result remains consistent even across different rows.

By default, strong consistency computation is not enabled. In this default mode, when the same data appears in different rows, the computation results may be inconsistent.

**Parameter values:**

- **0**: (Default value) Deterministic computation is not enabled.
- **1**: Deterministic computation is enabled.
- **2**: Strong consistency computation is enabled.
- **3**: Batch consistency computation is enabled.

**Configuration example:**

```c++
{ge::ir_option::DETERMINISTIC, "1"},
{ge::ir_option::DETERMINISTIC_LEVEL, "2"}
```

**Usage constraints:**

When building an offline model through the GE graph engine APIs, it is recommended to set the deterministic and consistency configurations through `ge::ir_option::DETERMINISTIC` and `ge::ir_option::DETERMINISTIC_LEVEL` during the model building phase. When loading and executing an offline model through the `aclmdl` APIs, it is not recommended to call `aclrtSetSysParamOpt` or `aclrtCtxSetSysParamOpt` to modify this configuration through `ACL_OPT_DETERMINISTIC`; otherwise, the runtime configuration may become inconsistent with the configuration used during model building, and the expected deterministic and consistency effects cannot be guaranteed. If this configuration needs to be adjusted, it is recommended to rebuild the offline model.

**Applicable products:**

<!-- npu="950" id15 -->
- Ascend 950PR&950DT series products: Supported
<!-- end id15 -->
<!-- npu="A3" id16 -->
- Atlas A3 series products: Supported
<!-- end id16 -->
<!-- npu="910b" id17 -->
- Atlas A2 series products: Supported
<!-- end id17 -->
<!-- npu="310b" id18 -->
- Atlas 200I/500 A2 inference products: Not supported
<!-- end id18 -->
<!-- npu="310p" id19 -->
- Atlas inference products: Supported
<!-- end id19 -->
<!-- npu="910" id20 -->
- Atlas training products: Supported
<!-- end id20 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/cpp/ge/aclgrphBuildInitialize_config_params/experimental_parameters_res.md#id1 -->

## OO\_CONSTANT\_FOLDING

**Extended parameter for the debugging feature. It is not supported yet for use in production environments and will be released as a formal feature update in a later version.**

Whether to enable constant folding optimization. The corresponding **options** parameter of this parameter is `ge.oo.constantFolding`.

Constant folding replaces nodes in the computation graph whose output values can be determined in advance with constants, and performs some structural simplification on the computation graph.

**Parameter value:**

- true: (default value) Enable.
- false: Disable.

**Configuration example:**

```c++
{ge::ir_option::OO_CONSTANT_FOLDING, "true"}
```

**Applicable products:**

Supported on all chips.

## OO\_DEAD\_CODE\_ELIMINATION

**Extended parameter for the debugging feature. It is not supported yet in production environments and will be released as a formal feature in a later version.**

Whether to enable dead edge elimination optimization. The actual **options** parameter corresponding to this parameter is `ge.oo.deadCodeElimination`.

Dead edge elimination: switch dead edge elimination. When the pred input (input 1) of a switch is a **const** node, one branch is eliminated based on the value of the **const**: when the **const** is **true**, the **false** branch is eliminated; when the **const** is **false**, the **true** branch is eliminated.

**Parameter value:**

- **true**: (default value) Enable.
- **false**: Disable.

**Configuration example:**

```c++
{ge::ir_option::OO_DEAD_CODE_ELIMINATION, "true"}
```

**Applicable products:**

Supported on all chips.

## OO\_LEVEL

**Extended parameter for debugging features. It is not supported yet for use in production environments and will be released as a formal feature in a later version.**

Multi-level optimization options for graph compilation, including subgraph optimization, whole-graph optimization, and static Shape model sinking. The actual options parameter corresponding to this parameter is `ge.oo.level`.

Static Shape model sinking: For a static Shape model, the input and output shapes of all operators can be determined at compile time, so that host-side computations such as model-level memory orchestration and operator Tiling computation are completed at compile time. The entire model is then delivered to the Device stream when the model is loaded, but is not executed immediately. Execution of all tasks in the model is triggered by delivering a model execution task.

**Parameter values:**

- **O1**: Disables all graph fusion and UB fusion passes, and enables only the optimizations that facilitate static sinking, such as **InferShape** (shape inference for output tensors), constant folding, and dead edge elimination.

  <!-- npu="950" id9 -->
  Ascend 950PR&950DT products do not support UB fusion, so only the graph fusion passes are disabled.
  <!-- end id9 -->

- O3: (Default value) Enables all optimizations.

**Usage constraints:**

When the value is O1, all graph fusion and UB fusion passes are disabled, and only the passes related to static sinking are enabled. However, the graph fusion passes in the file at the following path are enabled by default because disabling them would cause functional issues:

All graph fusion passes under the `"ExceptionalPassOfO1Level"` field in the `${INSTALL_DIR}/<arch>-linux/lib64/plugin/opskernel/fusion_pass/config/fusion_config.json` file.

Replace **$\{INSTALL\_DIR\}** with the file storage path after CANN software installation. Taking root user installation as an example, the default storage path after installation is **/usr/local/Ascend/cann**. **<arch\>** indicates the specific operating system architecture.

**Configuration example:**

```c++
{ge::ir_option::OO_LEVEL, "O3"}
```

**Applicable products:**

Supported on all chips.

## TUNE\_DEVICE\_IDS

**Not supported yet in the current version.**
