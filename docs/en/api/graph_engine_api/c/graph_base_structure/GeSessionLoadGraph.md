# GeSessionLoadGraph

<!-- md-trans-meta sourceCommit=7b50d660bdfbb603f65d2334f268aca0e63d2718 translatedAt=2026-10-27T16:30:55.462Z pushedAt=2026-10-28T13:33:57.668Z nl=2 -->

## Product Support

See [Session API Applicable Products](../../cpp/ge/Session/overview.md).

## Header File/Library File

- Header file: \#include <ge/ge\_api.h\>
- Library file: libge\_runner.so

## Feature Description

This API is used only in asynchronous graph execution scenarios. It binds the graph with the specified Graph ID to the corresponding stream using the specified session.

After GeSessionLoadGraph succeeds, you can use the [GeSessionExecuteGraphWithStreamAsync](GeSessionExecuteGraphWithStreamAsync.md) API to execute the graph.

## Prototype

```c
ge::Status GeSessionLoadGraph(ge::Session &session, uint32_t graph_id, const std::map<ge::AscendString, ge::AscendString> &options, void *stream)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| session | Input | Session instance for loading the graph. |
| graph_id | Input | ID of the graph to be executed. |
| options | Input | Options that may be used during the execution phase. A map whose key is the parameter type and whose value is the parameter value, describing the graph configuration information.<br>Generally, this can be left empty, consistent with the global options passed to GEInitialize.<br>The key and value types are AscendString. To configure the current graph separately, use this parameter. For supported configuration items, see [options parameter description](../../cpp/ge/options_params/options_parameters_description.md)>Basic Functions>ge.exec.frozenInputIndexes. Currently, only this parameter is supported. |
| stream | Input | Graph execution stream. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | Status | GE_CLI_SESS_RUN_FAILED: Serialization conversion failed when executing the subgraph.<br>SUCCESS: The execute subgraph succeeded.<br>FAILED: The execute subgraph failed. |

## Constraints

Before calling this API, complete the [CompileGraph](../../cpp/ge/Session/CompileGraph.md) process, and use this API together with [GeSessionExecuteGraphWithStreamAsync](GeSessionExecuteGraphWithStreamAsync.md).
