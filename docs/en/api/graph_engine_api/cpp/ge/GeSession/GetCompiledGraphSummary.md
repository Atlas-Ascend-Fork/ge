# GetCompiledGraphSummary

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:49:28.260Z pushedAt=2026-10-28T07:40:22.650Z nl=2 -->

## Header File/Library File

- Header file: \#include <ge/ge\_api\_v2.h\>
- Library file: libge\_runner\_v2.so

## Description

Queries the summary information of the graph compilation result, including the Feature memory size, Const memory size, number of Streams and Events, and whether the memory is refreshable.

- Before calling this API, you must call [CompileGraph](./CompileGraph.md) to compile the graph.
- You can allocate memory on your own and then set or update the Feature memory and Const memory base addresses through the following APIs:
    - Set the Const memory base address through [SetGraphConstMemoryBase](./SetGraphConstMemoryBase.md). The memory size is obtained from the **GetCompiledGraphSummary\>GetConstMemorySize** API.
    - Update the Feature memory base address through [UpdateGraphFeatureMemoryBase](./UpdateGraphFeatureMemoryBase.md). The memory size is obtained from the **GetCompiledGraphSummary\>GetFeatureMemorySize** API.
    - Set the Fixed Feature memory base addresses of different memory types of the graph through [SetGraphFixedFeatureMemoryBaseWithType](./SetGraphFixedFeatureMemoryBaseWithType.md). The memory size is obtained from the **GetCompiledGraphSummary\>GetAllFeatureMemoryTypeSize** API.
    - Update the refreshable Feature memory base address other than Fixed through [UpdateGraphRefreshableFeatureMemoryBase](./UpdateGraphRefreshableFeatureMemoryBase.md). The memory size is obtained from the **GetCompiledGraphSummary\>GetRefreshableFeatureMemorySize** API.

## Prototype

```c++
CompiledGraphSummaryPtr GetCompiledGraphSummary(uint32_t graph_id)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| graph_id | Input | ID of the graph. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | CompiledGraphSummaryPtr | **shared_ptr** of the graph compilation result summary [CompiledGraphSummary](../CompiledGraphSummary/CompiledGraphSummary.md). |

## Constraints

None
