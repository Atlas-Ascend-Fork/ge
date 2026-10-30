# Deprecated Configurations for Future Versions

<!-- md-trans-meta sourceCommit=6693584152164a17682ed389feb8e51961ab9533 translatedAt=2026-10-26T12:38:02.673Z pushedAt=2026-10-28T07:42:19.738Z nl=2 -->

## ge.exec.dataInputsShapeRange

**This parameter is deprecated and must not be used.**

Shape range of dynamic inputs. For example, if the entire graph has two data inputs, the configuration example is as follows:

```c++
std::map<ge::AscendString, ge::AscendString> ge_options = {{"ge.exec.deviceId", "0"},
      {"ge.graphRunMode", "1"},
      {"ge.exec.dynamicGraphExecuteMode", "dynamic_execute"},
      {"ge.exec.dataInputsShapeRange", "[128 ,3~5, 2~128, -1],[ 128 ,3~5, 2~128, -1]"}};
```

- The setting format is "\[n1,c1,h1,w1\],\[n2,c2,h2,w2\]", for example, "\[8\~20,3,5,-1\],\[5,3\~9,10,-1\]". You can omit the node name. By default, the first pair of brackets corresponds to the first input node, and nodes are separated by commas. When setting INPUT\_SHAPE\_RANGE by index, the data node must have the index attribute set to indicate which input it is, with the index starting from 0.
- For dynamic dimensions, a shape range is represented by a tilde “\~”, a fixed dimension is represented by a fixed number, and an unbounded range is represented by -1.
- For scalar inputs, a shape range must also be specified, in the format \[\].
- In a multi-input scenario, for example, when there are three inputs and only the second and third inputs have shape ranges while the first input is fixed, you still need to include the fixed input shape in the options field, for example:

    \{"ge.exec.dataInputsShapeRange", "\[3,3,4,10\], \[-1,3,2\~1000,-1\],\[-1,-1,-1,-1\]"\}\};

> [!NOTE]
>
>- If no node name is specified, nodes are stored in index order by default. The storage example is as follows: `xxx\_0, xxx\_1, xxx\_2, ...`, where the part after the underscore is the definition order index of the node in the network script. Nodes are arranged in alphabetical order of this index. Therefore, when the number of nodes is greater than 10, the order becomes `xxx_0 -> xxx_10 -> xxx_2 -> xxx_3`, where the node with definition index 10 in the network script is placed before the node with index 2, causing the defined shape range to mismatch the actual input node. To avoid this issue, when the number of input nodes is greater than 10, specify the node names in the network script so that nodes are named by the specified names, associating the shape range with the node names.
>
>- If this parameter is configured together with ge.dynamicDims, as shown in the following example, the ge.dynamicDims parameter (dynamic profiling) takes precedence over the ge.exec.dataInputsShapeRange parameter (dynamic shape range).
>
>   ```c++
>   std::map<ge::AscendString, ge::AscendString> ge_options =
>       {{"ge.inputShape", "data:1,1,40,-1;label:1,-1;mask:-1,-1" },
>        {"ge.dynamicDims", "20,20,1,1;40,40,2,2;80,60,4,4"},
>           xxx
>        {"ge.exec.dataInputsShapeRange", "[128, 3~5, 2~128, -1],[ 128 ,3~5, 2~128, -1]"}};
>   ```
>

**Required/Optional**: Optional

**Effective level**: graph

## ge.exec.dynamicGraphExecuteMode

**This parameter is deprecated and must not be used.**

For dynamic input scenarios, use this parameter to set the execution mode. The value is: dynamic\_execute.

**Required/Optional**: Optional

**Effective level**: graph

## ge.graphMemoryMaxSize

**This parameter is deprecated in future versions and must not be used.**

Specifies the static memory and maximum dynamic memory of the network, which can be set based on the network size. Unit: Byte. Value range: \[0, 256\*1024\*1024\*1024\] or \[0, 274877906944\]. Due to chip hardware limitations, the total of ge.graphMemoryMaxSize and ge.variableMemoryMaxSize supports a maximum of 31 GB. If not set, the default is 26 GB.

**Required/Optional**: Optional

**Effective level**: global/session/graph

## ge.opSelectImplmode

**This parameter is no longer evolving and will be deprecated in future versions. Use the ge.exec.op\_precision\_mode parameter instead.**

Some built-in operators on the AI processor have both high-precision and high-performance implementations. You can use this parameter to configure which operator implementation is selected during model compilation.

High precision means that in float16 input scenarios, the operator precision is further improved through methods such as Taylor expansion or Newton iteration. High performance means the optimal performance implementation that does not affect network precision in float16 input scenarios.

**Parameter values:**

- **high\_precision**: The operator uses the high-precision implementation mode.

    This option uses the system built-in configuration file to set the operator implementation mode. The built-in configuration file path is `${INSTALL_DIR}/opp/built-in/op_impl/ai_core/tbe/impl_mode/high_precision.ini`.

    For compatibility, this parameter takes effect only for the operator list in the high\_precision.ini file. This list controls the scope of operators affected by this parameter and ensures that network models from previous versions are not affected.

- **high\_performance**: (Default) The operator uses the high-performance implementation mode.

    This option uses the system built-in configuration file to set the operator implementation mode. The built-in configuration file path is `${INSTALL_DIR}/opp/built-in/op_impl/ai_core/tbe/impl_mode/high_performance.ini`.

    For compatibility, this parameter takes effect only on the operator list in the high\_performance.ini file. This list controls the scope of operators affected by the parameter and ensures that network models from previous versions are not affected.

- **high\_precision\_for\_all**: Indicates that the operator uses the high-precision implementation mode.

    This option uses the system built-in configuration file to set the operator implementation mode. The built-in configuration file path is `${INSTALL_DIR}/opp/built-in/op_impl/ai_core/tbe/impl_mode/high_precision_for_all.ini`, and the list in this file may be updated in future versions.

    **This implementation mode does not guarantee compatibility.** If an operator gains a new implementation mode in a future software package (that is, an implementation mode for an operator is added to the configuration file), the performance of network models that use high\_precision\_for\_all in previous versions may degrade on the new version.

- **high\_performance\_for\_all**: Indicates that the operator uses the high-performance implementation mode.

    This option uses the system built-in configuration file to set the operator implementation mode. The built-in configuration file path is `${INSTALL_DIR}/opp/built-in/op_impl/ai_core/tbe/impl_mode/high_performance_for_all.ini`, and the list in this file may be updated in future versions.

    **This implementation mode does not guarantee compatibility.** If an operator gains a new implementation mode in a future software package (that is, an implementation mode for an operator is added to the configuration file), the precision of network models that use high\_performance\_for\_all in previous versions may degrade on the new version.

The preceding implementation modes are distinguished by the dtype of the operator. Replace $\{INSTALL\_DIR\} with the file storage path after CANN software installation. For example, when installed as the root user, the default file storage path is /usr/local/Ascend/cann.

**Configuration example:**

```c++
{"ge.opSelectImplmode", "high_performance"};
```

**Required/Optional**: Optional

**Effective level**: global

## ge.optypelistForImplmode

Lists the operator optypes that use the mode specified by the ge.opSelectImplmode parameter.

**Parameter value constraints:**

- The operators in the list use the mode specified by the ge.opSelectImplmode parameter. Currently, only high\_precision and high\_performance are supported. Separate multiple operators with commas.
- This parameter must be used together with the ge.opSelectImplmode parameter and takes effect only on the specified operators. Operators that are not specified use the default implementation. For example, if ge.opSelectImplmode is set to high\_precision and ge.optypelistForImplmode is set to Pooling,SoftmaxV2, the Pooling and SoftmaxV2 operators use the unified high precision mode, while unspecified operators use their default implementation.

**Required/Optional**: Optional

**Effective level**: Global

## ge.shape\_generalized\_build\_mode

**This parameter is deprecated in future versions and must not be used.**

**Required/Optional**: Optional

**Effective level**: graph

## ge.variableMemoryMaxSize

**This parameter is deprecated in future versions and must not be used.**

Variable memory, which can be specified based on the network size. Unit: Byte. Value range: \[0, 256\*1024\*1024\*1024\] or \[0, 274877906944\]. Currently, due to chip hardware limitations, the total of ge.graphMemoryMaxSize and ge.variableMemoryMaxSize supports a maximum of 31 GB. If not set, the default is 5 GB.

**Required/Optional**: Optional

**Effective level**: global/session/graph
