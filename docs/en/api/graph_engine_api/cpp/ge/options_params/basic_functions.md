# Basic Functions

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-10-26T12:38:15.473Z pushedAt=2026-10-28T07:42:19.740Z nl=2 -->

## ge.constLifecycle

Configures the lifecycle of constant nodes in training and online inference scenarios.

**Parameter values:**

- session: Stores constant nodes at the session level. When configured at the session level, constant nodes across multiple graphs within a session support memory reuse, but you must ensure that constant nodes with the same name across multiple graphs are completely identical.
- graph: Stores constant nodes at the graph level. You can then call the [SetGraphConstMemoryBase](../Session/SetGraphConstMemoryBase.md) API to manage const memory at the graph level.

The default value is session in training scenarios and graph in online inference scenarios.

**Configuration example:**

```c++
{"ge.constLifecycle", "graph"};
```

**Required/Optional**: Optional

**Effective level**: Global/session/graph

## ge.deterministic

Whether to enable deterministic computation.

By default, deterministic computation is disabled, and an operator may produce different results across multiple executions on the same hardware and with the same input. This difference generally arises because the operator implementation involves asynchronous multi-threaded execution, which changes the order in which floating-point numbers are accumulated. When deterministic computation is enabled, the operator produces the same output across multiple executions on the same hardware and with the same input. However, enabling deterministic computation often slows down operator execution. You can enable deterministic computation to facilitate model debugging and tuning when a model produces different results across multiple executions or when you are performing precision tuning.

**Parameter values:**

- 0: (default value) Disables deterministic computation.
- 1: Enables deterministic computation.

**Configuration example:**

```c++
{"ge.deterministic", "0"};
```

**Required/Optional**: Optional

**Effective level**: Global

## ge.enableSingleStream

In static shape scenarios, whether to enable single-stream serial execution in graph running order.

A stream is used to maintain the execution order of asynchronous operations, ensuring that they are executed on the Device in the order in which the code is called in the application.

**Parameter values:**

- true: Enables single-stream serial execution in graph running order.
- false: (default value) Disables it, and the graph runs with multi-stream parallel execution.

**Configuration example:**

```c++
{"ge.enableSingleStream", "false"};
```

**Required/Optional**: Optional

**Effective level**: graph

**Usage constraints:**

When the model contains Cmo operators and the following control operators, the single-stream feature cannot be used, and only the default value false can be used.

- Merge
- Switch
- Enter
- RefEnter

## ge.exec.deviceId

Logical ID of the device on which the GE instance performs runtime operations.

**Parameter values:**

- In the online inference scenario, the value is limited to the finite range -1\~N-1, and the default value is -1.
- In the training scenario, the value is limited to the finite range 0\~N-1, and the default value is 0.

N indicates the number of available AI processors on the server.

**Configuration example:**

```c++
{"ge.exec.deviceId", "-1"};
```

**Required/Optional**: Optional

**Effective level**: Global

## ge.exec.frozenInputIndexes

Sets the index of the input tensor whose address is not refreshed. This parameter applies only to [LoadGraph](../Session/LoadGraph.md) calls. The content of the input tensor index varies by model:

- Dynamic shape model: You need to pass the index of the input tensor, the address of the data on the Device, and the data length, where the address must be in decimal notation.
- Static shape model: You only need to pass the index of the input tensor. Other parameters, such as the data length, do not take effect.

**Configuration example:**

```c++
# Pass only the input tensor index.
{"ge.exec.frozenInputIndexes", "0;1;2"};
# Pass the input tensor index, the address of the data on the Device, and the data length.
{"ge.exec.frozenInputIndexes", "0,88832131,4;1,888213294,4;2,193492421,2"};
```

For detailed usage examples and precautions, see [Single-Process Single-Card Asynchronous Running](../../../../../user_guides/graph_dev/compile_and_run_graph/run_graph_asynchronously.md#running-asynchronously-on-a-single-device-in-a-single-process).

**Required/Optional**: Optional

**Effective level**: graph

**Usage constraints**:

An input tensor whose address is not refreshed must have a static shape. For a dynamic shape model, the tensor of this input node must also have a static shape.

## ge.exec.hostInputIndexes

In the inline copy scenario, sets the indexes of input tensors whose placement is Host. Multiple input tensor indexes are separated by semicolons.

Inline copy means that as the operator addresses in the model are refreshed, the input tensor memory on the Host side is copied to the Device side together.

**Configuration example:**

```c++
{"ge.exec.hostInputIndexes", "0;1;2"};
```

**Required/Optional**: Optional

**Effective level**: graph

**Usage constraints:**

- This parameter applies only to static-shape models.
- The inline copy feature is not suitable for large-volume input data, as it may degrade performance.
- If the model has only one input, this parameter cannot be used together with ge.exec.frozenInputIndexes. If the model has multiple inputs, it cannot specify the same input as ge.exec.frozenInputIndexes.
- After this parameter is loaded through the options configuration item, subsequent execution APIs do not support [RunGraphAsync](../Session/RunGraphAsync.md).

## ge.exec.rankTableFile

Describes the cluster information involved in collective communication, including the organization information of servers, devices, and containers. Specify the path to the ranktable file, including the file path and file name.

**Configuration example:**

```c++
{"ge.exec.rankTableFile", "./yourfilepath/ranktable.json"};
```

**Required/Optional**: Optional

**Effective level**: global/session/graph

## ge.exec.rankId

Specifies the rank ID of the current process in the group. It is used for distributed training and collective communication to identify the current device in multi-device training. For a user-defined group, ranks are renumbered from 0 within the group. For the hccl world group, the rank ID is the same as the world rank ID. This parameter must be used together with ge.exec.rankTableFile.

**Parameter values:**

An integer in string form:

- World rank ID, which identifies the rank of the process in the hccl world group. Range: 0\~ (rank size-1).
- Local rank ID, which identifies the rank of the process within its server in the group. Range: 0\~ (local rank size-1).

**Configuration example:**

```c++
{"ge.exec.rankId", "0"};
```

**Required/Optional**: Optional

**Effective level**: Global/session/graph

## ge.graphRunMode

Graph execution mode.

**Parameter values:**

- 0: (default value) In the online inference scenario, set this parameter to 0.
- 1: In the training scenario, set this parameter to 1.

**Configuration example:**

```c++
{"ge.graphRunMode", "0"};
```

**Required/Optional**: Optional

**Effective level**: Global/session

## ge.outputDatatype

Specifies the output data type of the graph.

**Parameter values:**

- FP32
- FP16
- UINT8
- INT8
- UINT16
- INT16
- UINT32
- INT32
- UINT64
- INT64
- DOUBLE
<!-- npu="950" id1 -->
- HIF8: Only Ascend 950PR&950DT products support this type.
- FP8E5M2: Only Ascend 950PR&950DT products support this type.
- FP8E4M3FN: Only Ascend 950PR&950DT products support this type.
<!-- end id1 -->
After graph compilation, in the corresponding subgraph file, the above data types are represented as follows:

- DT\_FLOAT
- DT\_FLOAT16
- DT\_UINT8
- DT\_INT8
- DT\_UINT16
- DT\_INT16
- DT\_UINT32
- DT\_INT32
- DT\_UINT64
- DT\_INT64
- DT\_DOUBLE
<!-- npu="950" id8 -->
- DT\_HIFLOAT8
- DT\_FLOAT8\_E5M2
- DT\_FLOAT8\_E4M3FN
<!-- end id8 -->

**Parameter value constraint:**

If no specific data type is specified, the data type of the node (operator) that outputs the last layer of the graph is used. If a type is specified, the type specified by this parameter is used.

**Configuration example:**

```c++
{"ge.outputDatatype", "FP32"};
```

**Required/Optional**: Optional

**Effective level**: session/graph

## ge.session\_device\_id

When you need to run different models on different Devices through the same script, you can use this parameter to specify the logical ID of the Device.

Typically, you can create multiple threads, each running a different Session, and pass a different ge.session\_device\_id to each Session.

**Configuration example:**

```c++
{"ge.session_device_id", "0"};
```

**Required/Optional**: Optional

**Effective level**: session

## ge.socVersion

Specifies the AI processor model for compiling and optimizing the model.

For the value query method, see [--soc_version](../../../../../user_guides/atc_tools/CLI_options/--soc_version.md).

**Required/Optional**: Optional

**Effective Level**: Global/session/graph
