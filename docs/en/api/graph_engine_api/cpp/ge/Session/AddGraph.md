# AddGraph

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-26T13:12:34.997Z pushedAt=2026-10-28T07:42:19.792Z nl=2 -->

## Header File/Library File

- Header file: \#include <ge/ge\_api.h\>
- Library file: libge\_runner.so

## Description

Adds a graph to the Session. A unique graph ID is generated within the Session.

## Prototype

> [!NOTE]
> APIs with the string data type will be deprecated in later versions. You are advised to use APIs with a non-string data type.

```c++
Status AddGraph(uint32_t graph_id, const Graph &graph)
Status AddGraph(uint32_t graph_id, const Graph &graph, const std::map<std::string, std::string> &options)
Status AddGraph(uint32_t graph_id, const Graph &graph, const std::map<AscendString, AscendString> &options)
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| graph_id | Input | ID corresponding to the graph. |
| graph | Input | Graph to be loaded into the Session. |
| options | Input | Map table whose keys are parameter types and values are parameter values, both in string format, describing the graph configuration information.<br>Generally, this parameter can be left empty, in which case it is consistent with the global options passed to GEInitialize.<br>To configure the current graph separately, use this parameter. For supported configuration items, see the graph-level parameters in [options Parameter Description](../options_params/options_parameters_description.md). |

## Return Value

| Name | Type | Description |
| --- | --- | --- |
| - | Status | GE_CLI_GE_NOT_INITIALIZED: GE is not initialized.<br>SUCCESS: The graph is added successfully.<br>FAILED: The graph fails to be added. |

## Constraints

- If the same **Graph** object calls this API for registration, different **Graph ID**s actually share the same **Graph** object, causing subsequent operations to affect each other and result in errors.
- Do not use the same **Graph ID** to add different **Graph** objects. In this case, only the first added **Graph** object is retained, and subsequent **Graph** objects are not added successfully.
- When this API is used, the Session directly modifies the added graph object. To keep the original graph object unaffected after AddGraph, use the [AddGraphWithCopy](AddGraphWithCopy.md) API instead. AddGraphWithCopy copies the graph object in the Session and modifies only the copy of the graph object.
