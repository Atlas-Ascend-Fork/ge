# Experimental Parameters

<!-- md-trans-meta sourceCommit=9d821ee607484f54716ece49d9dbdbe1c4f9524d translatedAt=2026-10-26T12:41:13.397Z pushedAt=2026-10-28T07:42:19.745Z nl=2 -->

## ge.autoMultistreamParallelMode

**Extended parameter for debugging features. It is not supported yet for use in production environments and will be released as a formal feature in a later version.**

This parameter applies to static/dynamic shape graph scenarios. You can configure this parameter to control the automatic allocation strategy of the multi-stream parallel mode to improve graph execution performance.

**Parameter values:**

- cv: Enables parallel execution of Cube operators and Vector operators.
- LoadBalance:N: Load balancing algorithm that evenly distributes all operators across N streams for execution. N is the maximum number of streams, a positive integer in the range \[1,64\]. If N exceeds the number of actually available cores, performance may degrade.
- MainStream:N: Main stream algorithm that distributes serial operators to the main stream for execution and other parallelizable operators to other streams for execution. N is the maximum number of streams, a positive integer in the range \[1,64\]. If N exceeds the number of actually available cores, performance may degrade.
- None (default value): Disables all multi-stream parallel optimizations.

> [!NOTE]
>
>- This parameter is limited to recommended types of networks.
>- To use this feature with dynamic shape multi-stream, first enable dynamic shape multi-stream through [ENABLE\_DYNAMIC\_SHAPE\_MULTI\_STREAM](../../../../../user_guides/env_vars/ENABLE_DYNAMIC_SHAPE_MULTI_STREAM.md), and then configure this parameter.

**Configuration example:**

```c++
{"ge.autoMultistreamParallelMode", "cv"};
```

**Required/Optional**: Optional

**Effective level**: session/graph

## ge.build\_inner\_model

**Not supported in the current version.**

## ge.deterministicLevel

**An extended parameter of the debugging feature. It is not supported yet for use in production environments and will be released as a formal feature in a later version.**

Deterministic computation level.

By default, the deterministic level is 0, which means deterministic computation is disabled, and ge.deterministic, the option that controls whether to enable deterministic computation, must also be 0. To enable deterministic computation, set the deterministic level to 1 and set ge.deterministic to 1 as well. To enable strong consistency computation, set the level to 2 and set ge.deterministic to 1 as well. To enable Batch consistency computation, set the level to 3 and set ge.deterministic to 1 as well.

When strong consistency computation is enabled (ge.deterministicLevel=2), the computation result is deterministic, and repeated executions produce the same output. In addition, the computation result is independent of the data location. For example, during cube multiplication, the accumulation order may differ across rows, which may cause slight differences in the computation results of the same data in different rows. However, with strong consistency computation enabled, the computation results remain consistent as long as the inputs are the same, even across different rows.

By default, strong consistency computation is disabled. In this default mode, the same data in different rows may produce inconsistent computation results.

For performance reasons, it is generally recommended not to enable strong consistency computation, because it slows down operator computation and affects overall efficiency. Enable this feature only in the following scenarios: when you need to strictly guarantee result consistency for the same data at different locations, or when you are performing precision tuning and debugging on a model to optimize overall performance.

Parameter values:

- 0: (default value) Disable deterministic computation.
- 1: Enable deterministic computation.
- 2: Enable strong consistency computation.
- 3: Enable Batch consistency computation.

**Configuration example:**

```c++
{"ge.deterministic", "0"};
{"ge.deterministicLevel", "0"};
```

**Usage constraint:**

This configuration item must be used together with ge.deterministic.

**Required/Optional**: Optional

**Effective level**: Global

## ge.disableOptimizations

**This parameter is a debugging parameter. It is not supported yet for use in production environments and will be released as a formal feature in a later version.**

<!-- npu="A3,910b" id1 -->
This parameter applies only to the following products:

<!-- npu="910b" id2 -->
Atlas A2 series products
<!-- end id2 -->

<!-- npu="A3" id3 -->
Atlas A3 series products
<!-- end id3 -->
<!-- end id1 -->

Specifies one or more compilation optimization passes to **disable**.

Currently, only the following passes can be disabled:

```c++
"RemoveSameConstPass","ConstantFoldingPass","TransOpWithoutReshapeFusionPass"
```

Note:

1. Separate multiple passes with commas.
2. If you specify other passes to disable, they are not processed during graph compilation; only a warning-level log is printed.
3. If ConstantFoldingPass is disabled, graph compilation or execution may fail.
4. If other compilation optimization options are also configured, such as ge.oo.constantFolding, ge.disableOptimizations takes precedence.

**Configuration example**:

- Disable a single pass

    ```c++
    std::map <AscendString, AscendString> session_options = {
    {"ge.disableOptimizations", "RemoveSameConstPass"}
    };
    ```

- Disable multiple passes

    ```c++
    std::map <AscendString, AscendString> session_options = {
    {"ge.disableOptimizations", "RemoveSameConstPass, ConstantFoldingPass"}
    };
    ```

**Required/Optional**: Optional

**Effective level**: Global/session/graph

## ge.jit\_compile

**Extended parameter for the debugging feature. It is not supported yet for production environments and will be released as a formal feature in a later version.**

During model compilation, this parameter determines whether to prioritize online operator compilation or the use of precompiled operator binary files.

**Parameter values:**

- 0: Prioritize searching for precompiled operator binary files in the system. If found, the operators are not recompiled, resulting in better compilation performance. If not found, the operators are compiled.
- 1: Compile operators online. The system performs fusion and optimization based on the obtained graph information to compile operators with better runtime performance.
- 2: For static-shape networks, compile operators online. For dynamic-shape networks, prioritize searching for precompiled operator binaries in the system. If the corresponding binaries are not found, compile the operators.

<!-- npu="910,310p" id5 -->
**Usage constraint**

For the following product models, the default value of the `ge.jit\_compile` parameter is 1. For other products, the default value is 2.

<!-- npu="310p" id6 -->
Atlas inference products: supported
<!-- end id6 -->
<!-- npu="910" id7 -->
Atlas training products: supported
<!-- end id7 -->
<!-- end id5 -->

**Configuration example:**

```c++
{"ge.jit_compile", "1"};
```

**Required/Optional**: Optional

**Effective level**: Global/session

## ge.oo.constantFolding

**Extended parameter for debugging features. It is not supported yet for production environments and will be released as a formal feature in a later version.**

Whether to enable constant folding optimization.

Constant folding replaces nodes whose output values can be predetermined in the computation graph with constants, and performs some structural simplification on the computation graph.

**Parameter values:**

- true: (default value) Enable.
- false: Disable.

**Configuration example:**

```c++
{"ge.oo.constantFolding", "true"};
```

**Usage constraint:**

If other compilation optimization options are also configured, such as ge.disableOptimizations, ge.disableOptimizations takes precedence.

**Required/Optional**: Optional

**Effective level**: global/session/graph

## ge.oo.deadCodeElimination

**Extended parameter for the debugging feature. It is not supported yet for production environments and will be released as a formal feature in a later version.**

Whether to enable dead edge elimination optimization.

Dead edge elimination: switch dead edge elimination. When the pred input (input 1) of a switch is a const node, one branch is eliminated based on the value of the const: when const is true, the false branch is eliminated; when const is false, the true branch is eliminated.

**Parameter values:**

- true: (default value) Enable.
- false: Disable.

**Configuration example:**

```c++
{"ge.oo.deadCodeElimination", "true"};
```

**Required/Optional**: Optional

**Effective level**: Global/session/graph

## ge.oo.level

**Extended parameter for debugging features. It is not supported yet for production environments and will be released as a formal feature in a later version.**

Multi-level optimization options for graph compilation, including subgraph optimization, whole-graph optimization, and static shape model offloading.

Static shape model offloading: For a static shape model, the input and output shapes of all operators can be determined at compile time, allowing host-side computations such as model-level memory planning and operator Tiling computation to be completed. The entire model is then offloaded to the Device stream at model loading time, but is not executed immediately. Execution of all tasks in the model is triggered by issuing a model execution task.

**Parameter values:**

- O1: Disables all graph fusion and UB fusion passes, and performs only the optimizations that facilitate static offloading, such as InferShape (shape inference for output tensors), constant folding, and dead edge elimination.

    <!-- npu="950" id4 -->
    Ascend 950PR&950DT products do not support UB fusion, so only the graph fusion passes are disabled.
    <!-- end id4 -->

- O3: (Default value) Enables all optimizations.

**Usage constraint:**

When the value is O1, all graph fusion and UB fusion passes are disabled, and only the passes related to static offloading are enabled. However, the graph fusion passes in the file at the following path are enabled by default because disabling them would cause functional issues:

All graph fusion passes under the "ExceptionalPassOfO1Level" field in the `${INSTALL_DIR}/x86_64-linux/lib64/plugin/opskernel/fusion_pass/config/fusion_config.json` file.

Replace $\{INSTALL\_DIR\} with the file storage path after CANN software installation. For example, when installed as the root user, the default file storage path is /usr/local/Ascend/cann.

**Configuration example:**

```c++
{"ge.oo.level", "O3"};
```

**Required/Optional**: Optional

**Effective level**: Global/session/graph
