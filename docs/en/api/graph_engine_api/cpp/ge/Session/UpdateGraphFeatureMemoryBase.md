# UpdateGraphFeatureMemoryBase

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-26T13:27:33.003Z pushedAt=2026-10-28T07:42:19.837Z nl=2 -->

## Header File/Library File

- Header file: \#include <ge/ge\_api.h\>
- Library file: libge\_runner.so

## Description

Updates the Feature memory base address of a graph.

The memory size is obtained from the [GetCompiledGraphSummary](GetCompiledGraphSummary.md) \>**GetFeatureMemorySize** API.

Feature memory refers to the intermediate memory required during model execution (for example, the input and output memory of intermediate nodes).

## Prototype

```c++
Status UpdateGraphFeatureMemoryBase(uint32_t graph_id, const void *const memory, size_t size)
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| graph_id | Input | ID of the graph to be updated. |
| memory | Input | Feature memory base address to be set. |
| size | Input | Feature memory size to be set. |

## Return Value

| Name | Type | Description |
| --- | --- | --- |
| - | Status | **SUCCESS**: Set successfully.<br>**FAILED**: Failed to set. |

## Constraints

- Before calling this API, you must call the [CompileGraph](CompileGraph.md) API to compile the graph.
- After the graph is compiled by calling [CompileGraph](CompileGraph.md), you can call [GetCompiledGraphSummary](GetCompiledGraphSummary.md) to check whether the Feature address is refreshable. Only graphs whose Feature address is refreshable support repeated calls to this API to refresh the Feature address.
- This API applies only to statically compiled graphs. You can call the **IsStatic** API in [GetCompiledGraphSummary](GetCompiledGraphSummary.md) to check whether a graph is statically compiled.
- If this API is used and the [RegisterExternalAllocator](RegisterExternalAllocator.md) API is also configured, the RegisterExternalAllocator API does not take effect.
- This API cannot be used simultaneously with [SetGraphFixedFeatureMemoryBase](SetGraphFixedFeatureMemoryBase.md), nor with [SetGraphFixedFeatureMemoryBaseWithType](SetGraphFixedFeatureMemoryBaseWithType.md) when its input parameter is **MEMORY\_TYPE\_DEFAULT**.
- This API cannot be used simultaneously with [UpdateGraphRefreshableFeatureMemoryBase](UpdateGraphRefreshableFeatureMemoryBase.md).
