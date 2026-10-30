# GetCompiledGraphSummary

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-26T13:16:45.073Z pushedAt=2026-10-28T07:42:19.805Z nl=2 -->

## Header File/Library File

- Header file: \#include <ge/ge\_api.h\>
- Library file: libge\_runner.so

## Description

Queries the summary information of the graph compilation result, including the Feature memory size, Const memory size, number of Streams and Events, and whether the memory is refreshable.

- Before calling this API, you must call [CompileGraph](CompileGraph.md) to compile the graph.
- You can apply for memory on your own and then set or update the Feature memory and Const memory base addresses through the following APIs:
    - Set the Const memory base address through [SetGraphConstMemoryBase](SetGraphConstMemoryBase.md). The memory size is obtained from the **GetCompiledGraphSummary\>GetConstMemorySize** API.
    - Update the Feature memory base address through [UpdateGraphFeatureMemoryBase](UpdateGraphFeatureMemoryBase.md). The memory size is obtained from the **GetCompiledGraphSummary\>GetFeatureMemorySize** API.
    - Set the Fixed Feature memory base address through [SetGraphFixedFeatureMemoryBase](SetGraphFixedFeatureMemoryBase.md). The memory size is obtained from the **GetCompiledGraphSummary\>GetFixedFeatureMemorySize** API.
    - Set the Fixed Feature memory base addresses for different memory types of the graph through [SetGraphFixedFeatureMemoryBaseWithType](SetGraphFixedFeatureMemoryBaseWithType.md). The memory size is obtained from the **GetCompiledGraphSummary\>GetAllFeatureMemoryTypeSize** API.
    - Update the refreshable Feature memory base address (excluding Fixed) through [UpdateGraphRefreshableFeatureMemoryBase](UpdateGraphRefreshableFeatureMemoryBase.md). The memory size is obtained from the **GetCompiledGraphSummary\>GetRefreshableFeatureMemorySize** API.

## Prototype

```c++
CompiledGraphSummaryPtr GetCompiledGraphSummary(uint32_t graph_id)
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| graph_id | Input | ID of the graph. |

## Return Value

| Name | Type | Description |
| --- | --- | --- |
| - | CompiledGraphSummaryPtr | **shared_ptr** of the summary information of the graph compilation result [CompiledGraphSummary](../CompiledGraphSummary/overview.md). |

## Constraints

None
