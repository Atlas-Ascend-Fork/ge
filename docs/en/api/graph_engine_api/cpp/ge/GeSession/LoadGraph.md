# LoadGraph

<!-- md-trans-meta sourceCommit=0e5c73f3b4489260ae9f6461e1178f02c751dbca translatedAt=2026-10-16T13:52:00.155Z pushedAt=2026-10-28T07:40:22.658Z nl=2 -->

## Header File/Library File

- Header file: \#include <ge/ge\_api\_v2.h\>
- Library file: libge\_runner\_v2.so

## Description

Loads a graph and prepares for its execution, including applying for and managing resources such as the memory and compute streams required for graph execution.

Note: If [CompileGraph](./CompileGraph.md) is not called to complete graph compilation before this API is called, this API automatically calls **CompileGraph** to complete the compilation.

## Prototype

```c++
Status LoadGraph(const uint32_t graph_id, const std::map<AscendString, AscendString> &options, void *stream) const
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| graph_id | Input | ID of the graph to be executed. |
| options | Input | Options that may be used during the execution phase. A map table whose key is the parameter type and whose value is the parameter value, describing the graph configuration information.<br>Generally, this can be left empty and kept consistent with the global options passed to [GEInitializeV2](GEInitializeV2.md).<br>The key and value types are **AscendString**. If you need to configure the current graph separately, you can configure it through this parameter. For supported configuration items, see [options parameter description](../options_params/basic_functions.md)>Basic Functions>**ge.exec.frozenInputIndexes** and **ge.exec.hostInputIndexes**. Currently, only the preceding two parameters are supported. |
| stream | Input | Stream created by the **aclrtCreateStream** API. It can also be set to **nullptr**. When a valid value is passed, if tasks need to be dispatched to a stream during loading, they are dispatched to the specified stream.<br><br>  - If used together with the [RunGraphWithStreamAsync](./RunGraphWithStreamAsync.md) API, a valid value is recommended. In this case, the stream loaded through **LoadGraph** and the stream used by **RunGraphWithStreamAsync** at runtime are recommended to be the same stream. If they are not the same stream, after **LoadGraph**, call the stream synchronization API **aclrtSynchronizeStream** on the stream used for loading to complete synchronization.<br>  - If used together with the [RunGraph](./RunGraph.md) or [RunGraphAsync](./RunGraphAsync.md) API, **nullptr** is recommended to simplify the process. If a valid value is passed, call **aclrtSynchronizeStream** after **LoadGraph** and before **RunGraph**/**RunGraphAsync** to complete stream synchronization and ensure that the loading task is complete.<br><br>For details about the API, see "Stream Management" in *Runtime API*. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | Status | **GE_CLI_SESS_RUN_FAILED**: Serialization conversion failed during graph execution.<br>**SUCCESS**: Graph execution succeeded.<br>**FAILED**: Graph execution failed. |
