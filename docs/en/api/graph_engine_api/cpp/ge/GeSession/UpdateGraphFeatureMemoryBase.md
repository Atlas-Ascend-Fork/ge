# UpdateGraphFeatureMemoryBase

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:56:50.482Z pushedAt=2026-10-28T07:40:22.671Z nl=2 -->

## Header File/Library File

- Header file: \#include <ge/ge\_api\_v2.h\>
- Library file: libge\_runner\_v2.so

## Description

Updates the Feature memory base address of a Graph.

Before calling this API, you must call [CompileGraph](./CompileGraph.md) to compile the graph. The memory size is obtained from the [GetCompiledGraphSummary](./GetCompiledGraphSummary.md)\>**GetFeatureMemorySize** API.

Feature memory refers to the intermediate memory required during model execution, such as the input and output memory of intermediate nodes.

## Prototype

```c++
Status UpdateGraphFeatureMemoryBase(uint32_t graph_id, const void *const memory, size_t size)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| graph_id | Input | ID of the Graph. |
| memory | Input | Base address of the Feature memory to be set. |
| size | Input | Size of the Feature memory to be set. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | Status | **SUCCESS**: Set successfully.<br>**FAILED**: Failed to set. |

## Constraints

- After calling CompileGraph to complete graph compilation, you can call the GetCompiledGraphSummary API to check whether the Feature address is refreshable. Only graphs whose Feature address is refreshable support repeated calls to this API to refresh the Feature address.
- This API applies only to statically compiled graphs. You can call the **IsStatic** API in GetCompiledGraphSummary to check whether a graph is statically compiled.
- If this API is used and the [RegisterExternalAllocator](./RegisterExternalAllocator.md) API is also configured, the RegisterExternalAllocator API does not take effect.
- It cannot be used together with the [SetGraphFixedFeatureMemoryBaseWithType](./SetGraphFixedFeatureMemoryBaseWithType.md) API when the input parameter is **MEMORY\_TYPE\_DEFAULT**.
- It cannot be used together with [UpdateGraphRefreshableFeatureMemoryBase](./UpdateGraphRefreshableFeatureMemoryBase.md).
