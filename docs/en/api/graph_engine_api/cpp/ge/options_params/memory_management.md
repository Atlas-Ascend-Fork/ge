# Memory Management

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-10-26T12:44:24.301Z pushedAt=2026-10-28T07:42:19.748Z nl=2 -->

## ge.exec.atomicCleanPolicy

Whether to centrally clean the memory occupied by all memset operators in the network (all operators with the memset attribute are memset operators).

**Parameter values**:

- 0: (default value) Centrally clean.
- 1: Clean individually. Each memset operator in the network is cleaned separately. This cleaning method is recommended when the memory occupied by memset operators in the network is too large, as it significantly reduces memory usage, but it may cause some performance loss.

**Configuration example:**

```c++
{"ge.exec.atomicCleanPolicy", "0"};
```

**Required/Optional**: Optional

**Effective level**: session

## ge.exec.disableReuseMemory

Memory reuse switch.

**Parameter values:**

- 1: Disable memory reuse.
- 0: (Default value) Enable memory reuse.

**Configuration example:**

```c++
{"ge.exec.disableReuseMemory", "0"};
```

**Required/Optional**: Optional

**Effective level**: global/session/graph

## ge.exec.input\_fusion\_size

Threshold for merging multiple discrete user inputs into a single copy when transferring host-side input data to the device side. The minimum value is 0 Byte, the maximum value is 33554432 Byte (32 MB), and the default value is 131072 Byte (128 KB). If:

- If the input data size is less than or equal to the threshold, the inputs are merged and then transferred from the host to the device.
- If the input data size is greater than the threshold, or the threshold is 0 (the function is disabled), the inputs are not merged and are transferred directly from the host to the device.

For example, a user has 10 inputs, of which 2 inputs are 100 KB in size, 2 inputs are 50 KB in size, and the remaining inputs are larger than 100 KB. If the following is set:

- ge.exec.input\_fusion\_size=100KB, the above 4 inputs are merged into 300 KB and transferred; the other 6 inputs are transferred directly from the host to the device.
- ge.exec.input\_fusion\_size=0KB, the function is disabled and no inputs are merged, that is, the 10 inputs are transferred directly from the host to the device.

This parameter takes effect only when the graph is executed in asynchronous static shape mode, that is, when the Graph is run using the [RunGraphAsync](../Session/RunGraphAsync.md) API.

**Configuration example:**

```c++
{"ge.exec.input_fusion_size", "131072"};
```

**Required/Optional**: Optional

**Effective level**: global/session/graph

## ge.exec.inputReuseMemIndexes

Configures whether to enable memory reuse for the input nodes of a graph. When enabled, the memory of the input nodes can be reused as intermediate memory required during model execution, thereby reducing the peak memory usage.

The parameter value is the index of the input node. To enable memory reuse for multiple input nodes, separate multiple indexes with commas. The input node must have the index attribute set to indicate which input it is, with the index starting from 0.

Note:

- If memory reuse is enabled for an input node, you must ensure that the input memory size is 32-byte aligned.
- If the configured input index is greater than or equal to the value of "number of inputs", it is an invalid index and the configuration for that index does not take effect.
- If memory reuse is enabled for an input node, the data in the input node memory will be overwritten. After graph execution, the content in the input node memory cannot be used further.

**Configuration example:**

```c++
{"ge.exec.inputReuseMemIndexes", "0,1,2"};
```

**Required/Optional**: Optional

**Effective level**: graph

## ge.exec.reuseZeroCopyMemory

Switch for zero-copy memory reuse.

In AI inference, the traditional approach requires copying data from host memory to device memory and then transferring it back after inference completes. This Host-to-Device and Device-to-Host copying is the primary source of inference latency. Zero-copy aims to eliminate intermediate copying and enable direct mapped read/write between user buffers and device memory.

By default, the zero-copy I/O memory region is reallocated on every model execution. When this option is enabled, GE plans the zero-copy memory blocks for model input and output independently, without merging them with other memory blocks, and reuses this region across multiple executions to avoid repeated memory allocation, thereby significantly reducing inference latency.

**Parameter values:**

- 0: (default value) Disabled. Zero-copy memory is not reused and is reallocated on every execution.
- 1: Enabled. Zero-copy memory is reused, and input/output I/O memory is reused across executions.

**Configuration example:**

```c++
{"ge.exec.reuseZeroCopyMemory", "0"};
```

**Required/Optional:**
Optional

**Effective level:**
Global/session/graph

## ge.exec.staticMemoryPolicy

Memory allocation method used during network running.

**Parameter values:**

- 0: (default value) Dynamically allocates memory based on the actual size. Memory expansion is not supported.
- 2: Static-shape memory dynamic expansion. In training and online inference scenarios, this value enables memory reuse across multiple graphs in the same session, that is, memory is allocated based on the maximum memory required by the graphs. For example, if the memory required by the currently executed graph exceeds that of the previous graph, the memory of the previous graph is released and reallocated based on the memory required by the current graph.
- 3: Only dynamic shapes support memory dynamic expansion, which resolves the fragmentation issue during dynamic memory allocation and reduces the memory usage of dynamic-shape networks.
- 4: Both static shapes and dynamic shapes support memory dynamic expansion.

> [!NOTE]
>
>- When multiple graphs are executed concurrently, configuring the value to "2" or "4" is not supported.
>- For compatibility with historical version configurations, when the value is set to "1", the system processes it in the same way as "2" for dynamic memory expansion.
>- When the value is set to "3" or "4", memory benefits are achieved, but certain performance loss may occur.

**Configuration example:**

```c++
{"ge.exec.staticMemoryPolicy", "0"};
```

**Required/Optional**: Optional

**Effective level**: global/session

## ge.exec.outputReuseInputMemIndexes

During model inference, explicitly declares that the specified output tensors share the same memory address as the input tensors.

When you can determine the memory reuse relationship before graph compilation, you can use this parameter to inform the GE graph compiler, which then performs the corresponding optimization during compilation. Typical scenarios are as follows:

Data\>TensorMove\>InplaceOp(in-place operator)\>NetOutput, where Data and NetOutput are the model input and output, respectively. If the lifecycles of Data and NetOutput meet the reuse conditions, the compiler automatically removes the intermediate TensorMove (memory copy operator), thereby avoiding unnecessary memory copy operations and improving model runtime performance.

**Parameter values:**

"input\_index\_0,output\_index\_0|input\_index\_1,output\_index\_1|..."

- Each pair of input\_index\_X,output\_index\_Y indicates that the input tensor with **input index X** and the output tensor with **output index Y** share the same memory address.
- Multiple pairs of indexes are separated by a vertical bar "|".

**Configuration example:**

Suppose a model has 10 inputs and 10 outputs, and the configuration is as follows:

```c++
ge.exec.outputReuseInputMemIndexes = "0,1|6,8"
```

This indicates that output index 1 and input index 0 share the same memory address, and output index 8 and input index 6 share the same memory address.

Description:

- Index validity: The declared input/output indexes must fall within the valid range defined by the model.
- Address consistency: During model execution, the corresponding input and output must be passed the same memory address; otherwise, a runtime error occurs.

**Required/Optional**: Optional

**Effective level**: graph

## ge.exec.outputReuseMemIndexes

Configures whether to enable memory reuse for whole-graph output. When enabled, the memory of the whole-graph output can be reused as intermediate memory required during model execution, thereby reducing the peak memory usage.

If enabled, configure the index of the whole-graph output. To enable memory reuse for multiple outputs, separate multiple indexes with commas.

Note:

- If memory reuse for whole-graph output is enabled, you must ensure that the output memory size is 32-byte aligned.
- Output indexes are identified in the order of the whole-graph output, starting from 0.
- If the configured output index is greater than or equal to the number of outputs, it is an invalid index and the configuration does not take effect.

**Configuration example:**

```c++
{"ge.exec.outputReuseMemIndexes", "0,1,2"};
```

**Required/Optional**: Optional

**Effective level**: graph

## ge.externalWeight

When multiple models are loaded simultaneously in the same session, if weights can be reused across the models, you are advised to use this configuration item to externalize the weights of Const/Constant nodes in the graph, so that weights can be reused across multiple models and the memory occupied by weights is reduced.

**Parameter values:**

- 0: (default value) Weights are not externalized and are directly stored in the graph.
- 1: Weights are externalized but not normalized. The weight files of all Const/Constant nodes in the graph are written to disk, and the node type is converted to FileConstant. Weights of different nodes are saved to different files, named by the weight\_\<hash value \>.
- 2: Weights are externalized and normalized. The weight files of all Const/Constant nodes in the graph are written to disk, and the node type is converted to FileConstant. All weights are saved to the same file, named by the "original root graph name \_weight\_combined”.

Description of the disk write path:

- If ge.externalWeightDir is configured, the weight files are written to the specified directory.
- If the environment variable ASCEND\_WORK\_PATH is not configured in the environment, the weight files are written to the current execution directory `tmp_weight_<pid>_<sessionid>`.
- If the environment variable ASCEND\_WORK\_PATH is configured in the environment, the weight files are written to the `${ASCEND_WORK_PATH}/tmp_weight_<pid>_<sessionid>` directory. For details about ASCEND\_WORK\_PATH, see [Environment Variable Reference](https://gitcode.com/cann/docs/blob/9.2.0/docs/en/env-vars/README.md).

Disk write path priority: ge.externalWeightDir \> `${ASCEND_WORK_PATH}/tmp_weight_<pid>_<sessionid>` \> the `tmp_weight_<pid>_<sessionid>` directory in the current execution directory.

When the model is unloaded, the `tmp_weight_<pid>_<sessionid>` directory is deleted.

**Configuration example:**

```c++
{"ge.externalWeight", "1"};
```

**Required/Optional**: Optional

**Effective level**: global/session

## ge.externalWeightDir

Configures the path for saving external weight files to disk.

**Usage constraints:**

- To specify a custom path for external weight files, use this parameter together with ge.externalWeight.
- Priority: ge.externalWeightDir \> `${ASCEND_WORK_PATH}/tmp_weight_<pid>_<sessionid>` \> `tmp_weight_<pid>_<sessionid>` in the current execution directory.

**Configuration example:**

```c++
{"ge.externalWeight", "1"};
{"ge.externalWeightDir", "$HOME/your_tmp_path"};
```

**Required/Optional**: Optional

**Effective level**: global/session

## ge.inputBatchCpy

Whether to enable batch memory copy when transferring input data from the host to the device.

This parameter improves the performance of host-to-device data transfer. It applies to scenarios where data is transferred frequently and PCIe bandwidth utilization is low. Enabling batch copy improves bandwidth utilization.

**Parameter values:**

- 1: Enables batch memory copy. This value takes effect only when the number of user inputs is greater than 1.
- 0: (Default value) Disables batch memory copy.

**Usage constraints:**

<!-- npu="950,A3,910b" id1 -->
- This parameter is supported only on the following products:

    Ascend 950PR&950DT products

    Atlas A3 products

    Atlas A2 products
<!-- end id1 -->

- This feature is typically used in multi-session scenarios. Because the number of inputs may differ across sessions, it is recommended to configure it at the session level and enable it on demand based on the inputs. Configuring it at the global level or graph level is not recommended.
- This parameter is passed in during session initialization. When running a graph later, the feature can currently be enabled only through the [RunGraphAsync](../Session/RunGraphAsync.md) API.
- If the initial number of network inputs is 1, the batch copy feature does not take effect even if it is configured.
- If both ge.exec.input\_fusion\_size merged copy and the ge.inputBatchCpy batch copy feature are enabled, the merged copy threshold may affect the batch copy feature.

    For example, if a user has five inputs and four of them meet the merged copy threshold, these four inputs are processed by the merged copy feature, while the remaining one input is not processed by the batch copy feature.

**Configuration example:**

```c++
{"ge.inputBatchCpy", "0"};
```

**Required/Optional**: Optional

**Effective level**: global/session/graph

## ge.featureBaseRefreshable

Configures whether the feature memory address is refreshable. If you need to manage the feature memory yourself and refresh the address multiple times, set this parameter to refreshable.

This parameter applies only to static-shape graphs.

**Parameter values:**

0: (default value) The feature memory address is not refreshable.

1: Supports refreshing the feature memory address of the model.

**Configuration example:**

```c++
{"ge.featureBaseRefreshable", "0"};
```

**Required/Optional**: Optional

**Effective level**: global/session/graph
