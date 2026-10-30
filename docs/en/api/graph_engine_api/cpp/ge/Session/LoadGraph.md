# LoadGraph

<!-- md-trans-meta sourceCommit=0e5c73f3b4489260ae9f6461e1178f02c751dbca translatedAt=2026-10-26T13:18:31.554Z pushedAt=2026-10-28T07:42:19.813Z nl=2 -->

## Header File/Library File

- Header file: \#include <ge/ge\_api.h\>
- Library file: libge\_runner.so

## Description

Used in asynchronous graph execution scenarios to bind the graph with the specified Graph ID to the corresponding stream.

Before calling this API, complete the [CompileGraph](CompileGraph.md) process. After LoadGraph succeeds, execute the graph using the [ExecuteGraphWithStreamAsync](ExecuteGraphWithStreamAsync.md) API.

## Prototype

```c++
Status LoadGraph(const uint32_t graph_id, const std::map<AscendString, AscendString> &options, void *stream) const
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| graph_id | Input | ID of the graph to be executed. |
| options | Input | Options that may be used during the execution phase. A map whose keys are parameter types and values are parameter values, describing the graph configuration information.<br>Generally, this parameter can be left empty and is consistent with the global options passed to [GEInitialize](GEInitialize.md).<br>The key and value types are AscendString. To configure the current graph separately, use this parameter. For supported configuration items, see [options parameter description](../options_params/options_parameters_description.md) > Basic Functions > **ge.exec.frozenInputIndexes** and **ge.exec.hostInputIndexes**. Currently, only these two parameters are supported. |
| stream (asynchronous graph execution scenario) | Input | Stream for graph execution. |

## Return Value

| Name | Type | Description |
| --- | --- | --- |
| - | Status | **GE_CLI_SESS_RUN_FAILED**: Serialization conversion failed during graph execution.<br>**SUCCESS**: Graph execution succeeded.<br>**FAILED**: Graph execution failed. |

## Constraints

The stream loaded through LoadGraph and the stream used when running through the [ExecuteGraphWithStreamAsync](ExecuteGraphWithStreamAsync.md) API are recommended to be the same stream. If they are not the same stream, after LoadGraph, call the stream synchronization API "aclrtSynchronizeStream" on the stream used for loading to complete synchronization.

For details about the API, see "Stream Management" in *Runtime API*.
