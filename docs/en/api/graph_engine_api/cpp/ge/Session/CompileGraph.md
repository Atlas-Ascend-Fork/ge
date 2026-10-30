# CompileGraph

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-26T13:13:29.708Z pushedAt=2026-10-28T07:42:19.793Z nl=2 -->

## Header File/Library File

- Header file: \#include <ge/ge\_api.h\>
- Library file: libge\_runner.so

## Description

Synchronously compiles the Graph corresponding to the specified ID. Compared with [BuildGraph](BuildGraph.md), this API only includes the graph compilation function and does not generate a model that can be used for execution. BuildGraph includes the graph compilation process and, after compilation is complete, initializes the memory resources required by the model to generate a model that can be used for execution.

- This API does not include the memory resource management function required by the model. Instead, it leaves this memory management work to you. You can use it together with the API for querying the resource usage of the compiled graph and the API for refreshing the memory base address, so as to manage the model memory by yourself and gain more flexibility:

    After calling this API, call [GetCompiledGraphSummary](GetCompiledGraphSummary.md) to obtain the summary information of the graph compilation result (for example, the size of the memory resources required for model execution and whether the memory can be refreshed or reused). Based on the queried memory size, apply for and manage the memory by yourself. Then use [SetGraphConstMemoryBase](SetGraphConstMemoryBase.md) and [UpdateGraphFeatureMemoryBase](UpdateGraphFeatureMemoryBase.md) to set and refresh the memory base address.

- This API can only be used together with [RunGraphWithStreamAsync](RunGraphWithStreamAsync.md) or [ExecuteGraphWithStreamAsync](ExecuteGraphWithStreamAsync.md). It does not support use with [RunGraph](RunGraph.md) or [RunGraphAsync](RunGraphAsync.md).
- Graphs containing Variable operators do not support this API.

## Prototype

```c++
Status CompileGraph(uint32_t graph_id)
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| graph_id | Input | ID corresponding to the Graph. |

## Return Value

| Name | Type | Description |
| --- | --- | --- |
| - | Status | **SUCCESS**: Graph compilation succeeded.<br>**FAILED**: Graph compilation failed.<br>**PARAM_INVALID**: The input graph information is abnormal during graph compilation. |

## Constraints

None
