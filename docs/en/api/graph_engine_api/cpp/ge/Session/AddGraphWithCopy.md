# AddGraphWithCopy

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-26T13:12:38.476Z pushedAt=2026-10-28T07:42:19.791Z nl=2 -->

## Header File/Library File

- Header file: \#include <ge/ge\_api.h\>
- Library file: libge\_runner.so

## Description

Adds a Graph to a Session, and a unique Graph ID is generated within the Session.

Note that, compared with the AddGraph API, this API creates a copy of the Graph object after the Graph object is passed in. The graph saved in the Session is a backup of the Graph object. Subsequent modifications to the Graph do not affect the original Graph in the Session, and any modifications to the graph in the Session do not affect the Graph object.

## Prototype

```c++
Status AddGraphWithCopy(uint32_t graph_id, const Graph &graph)
Status AddGraphWithCopy(uint32_t graph_id, const Graph &graph, const std::map<AscendString, AscendString> &options)
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| graph_id | Input | ID corresponding to the Graph. |
| graph | Input | Graph to be loaded into the Session. |
| options | Input | Map whose keys are parameter types and values are parameter values, both in string format, describing the Graph configuration information.<br>Generally, this can be left empty and kept consistent with the global options passed to [GEInitialize](GEInitialize.md).<br>To configure the current Graph separately, use this parameter. The supported configuration items are the same as those of GEInitialize. |

## Return Value

| Name | Type | Description |
| --- | --- | --- |
| - | Status | GE_CLI_GE_NOT_INITIALIZED: GE is not initialized.<br>SUCCESS: The graph is added successfully.<br>FAILED: The graph fails to be added. |

## Constraints

Calling this API to register Graphs of the same object results in different Graph IDs corresponding to different backups. The backups corresponding to two different Graph IDs are not shared.
