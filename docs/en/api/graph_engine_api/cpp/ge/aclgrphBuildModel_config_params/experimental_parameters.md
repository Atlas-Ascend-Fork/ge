# Experimental Parameters

## BUILD\_INNER\_MODEL

Not supported in the current version.

## OO\_CONSTANT\_FOLDING

**Extended parameter for the debugging feature. It is not supported yet for use in production environments and will be released as a formal feature in a later version.**

Whether to enable constant folding optimization. The corresponding **options** parameter of this parameter is `ge.oo.constantFolding`.

Constant folding replaces nodes whose output values can be predetermined in the computation graph with constants, and performs some structural simplification on the computation graph.

**Parameter values:**

- **true**: (default value) Enable.
- **false**: Disable.

**Configuration example:**

```c++
{ge::ir_option::OO_CONSTANT_FOLDING, "true"}
```

**Applicable products:**

Supported on all chips.

## OO\_DEAD\_CODE\_ELIMINATION

**Debugging feature extended parameter. It is not supported yet for production environments and will be released as a formal feature in a later version.**

Whether to enable dead edge elimination optimization. The actual **options** parameter corresponding to this parameter is `ge.oo.deadCodeElimination`.

Dead edge elimination: switch dead edge elimination. When the pred input (input 1) of a switch is a const node, one branch is eliminated based on the value of the const: when the const is true, the false branch is eliminated; when the const is false, the true branch is eliminated.

**Parameter values:**

- true: (default value) enable.
- false: disable.

**Configuration example:**

```c++
{ge::ir_option::OO_DEAD_CODE_ELIMINATION, "true"}
```

**Applicable products:**

Supported on all chips.

## OO\_LEVEL

**Extended parameter for the debugging feature. It is not supported yet for use in production environments and will be released as a formal feature in a later version.**

Multi-level graph compilation optimization options, including subgraph optimization, whole-graph optimization, and static Shape model sinking. The actual **options** parameter corresponding to this parameter is `ge.oo.level`.

Static Shape model sinking: For a static Shape model, the input and output Shapes of all operators can be determined at compilation time, so that Host-side computations such as model-level memory layout and operator Tiling calculation are completed. During model loading, the model is delivered to the Device stream as a whole but is not executed immediately. Execution of all Tasks in the model is triggered by delivering a model execution Task.

**Parameter values:**

- **O1**: Disables all graph fusion and UB fusion passes, and only performs optimizations that facilitate static sinking, such as **InferShape** (shape inference of output tensors), constant folding, and dead edge elimination.

    The Ascend 950PR&950DT products does not support UB fusion, so only graph fusion passes are disabled.

- **O3**: (Default value) Enables all optimizations.

**Constraints:**

When the value is **O1**, all graph fusion and UB fusion passes are disabled, and only the passes related to static sinking are enabled. However, the graph fusion passes in the file at the following path are enabled by default because disabling them causes functional issues:

All graph fusion passes under the "ExceptionalPassOfO1Level" field in the `${INSTALL_DIR}/<arch>-linux/lib64/plugin/opskernel/fusion_pass/config/fusion_config.json` file.

Here, **$\{INSTALL\_DIR\}** must be replaced with the file storage path after CANN software installation. Taking installation by the root user as an example, the default file storage path after installation is **/usr/local/Ascend/cann**. **<arch\>** indicates the specific operating system architecture.

**Configuration example:**

```c++
{ge::ir_option::OO_LEVEL, "O3"}
```

**Applicable products:**

Supported on all chips.
