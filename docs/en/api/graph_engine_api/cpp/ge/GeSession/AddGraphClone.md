# AddGraphClone

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:46:25.094Z pushedAt=2026-10-28T07:40:22.639Z nl=2 -->

## Header File/Library File

- Header file: \#include <ge/ge\_api\_v2.h\>
- Library file: libge\_runner\_v2.so

## Description

Adds a **Graph** to the **GeSession**, and a unique Graph ID is generated within the **GeSession**.

Compared with the [AddGraph](./AddGraph.md) API, this API creates a copy of the **Graph** object after the **Graph** object is passed in. The graph saved in the **GeSession** is a backup of the **Graph** object. Subsequent modifications to the **Graph** do not affect the original graph in the **GeSession**, and any modifications to the graph in the **GeSession** do not affect the **Graph** object either.

## Prototype

```c++
Status AddGraphClone(uint32_t graph_id, const Graph &graph)
Status AddGraphClone(uint32_t graph_id, const Graph &graph, const std::map<AscendString, AscendString> &options)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| graph_id | Input | ID corresponding to the **Graph**. |
| graph | Input | **Graph** to be loaded into the **GeSession**. |
| options | Input | A map whose **key**s are parameter types and **value**s are parameter values, both in string format, describing the Graph configuration information.<br>Generally, this parameter can be left empty and kept consistent with the global options passed to [GEInitializeV2](GEInitializeV2.md).<br>To configure the current Graph separately, use this parameter. The supported configuration items are the same as those of **AddGraph**. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | Status | **GE_CLI_GE_NOT_INITIALIZED**: GE is not initialized.<br>**SUCCESS**: The graph is added successfully.<br>**FAILED**: The graph fails to be added. |

## Constraints

Registering the same **Graph** object through this API results in different Graph IDs corresponding to different backups, and the backups corresponding to two different Graph IDs are not shared.
