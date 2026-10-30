# AddGraph

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:46:16.110Z pushedAt=2026-10-28T07:40:22.638Z nl=2 -->

## Header File/Library File

- Header file: \#include <ge/ge\_api\_v2.h\>
- Library file: libge\_runner\_v2.so

## Description

Adds a Graph to the **GeSession**, and a unique Graph ID is generated within the **GeSession**.

## Prototype

```c++
Status AddGraph(uint32_t graph_id, const Graph &graph)
Status AddGraph(uint32_t graph_id, const Graph &graph, const std::map<AscendString, AscendString> &options)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| graph_id | Input | ID corresponding to the Graph. |
| graph | Input | Graph to be loaded into the **GeSession**. |
| options | Input | Map whose keys are parameter types and values are parameter values, both in string format, describing the Graph configuration information.<br>Generally, this parameter can be left empty, in which case it is consistent with the global options passed to **GEInitializeV2**.<br>If you need to configure the current Graph separately, you can configure it through this parameter. For the supported configuration items, see the graph-level parameters in [options Parameter Description](../options_params/options_parameters_description.md). |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | Status | **GE_CLI_GE_NOT_INITIALIZED**: GE is not initialized.<br>**SUCCESS**: The graph is added successfully.<br>**FAILED**: The graph fails to be added. |

## Constraints

- If the same **Graph** object calls this API for registration, different **Graph ID**s actually share the same **Graph** object, causing subsequent operations to affect each other and result in errors.
- Do not use the same **Graph ID** to add different **Graph** objects. In this case, only the first added **Graph** object is retained, and subsequent **Graph** objects are not added successfully.
- When this API is used, the **GeSession** directly modifies the added Graph object. If the original Graph object needs to remain unaffected after **AddGraph**, use the [AddGraphClone](AddGraphClone.md) API instead. **AddGraphClone** copies the Graph object in the Session and modifies only the copy of the Graph object.
