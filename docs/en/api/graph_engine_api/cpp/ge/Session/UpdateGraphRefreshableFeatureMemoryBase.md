# UpdateGraphRefreshableFeatureMemoryBase

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-26T13:28:32.105Z pushedAt=2026-10-28T07:42:19.839Z nl=2 -->

## Header File/Library File

- Header file: \#include <ge/ge\_api.h\>
- Library file: libge\_runner.so

## Description

Updates the refreshable Feature memory base address except for Fixed.

The memory size is obtained from the [GetCompiledGraphSummary](GetCompiledGraphSummary.md) \> **GetRefreshableFeatureMemorySize** API.

If the graph contains Fixed memory and you have not called the [SetGraphFixedFeatureMemoryBase](SetGraphFixedFeatureMemoryBase.md) or [SetGraphFixedFeatureMemoryBaseWithType](SetGraphFixedFeatureMemoryBaseWithType.md) API to set the Fixed Feature memory base address of the default memory type, Fixed memory is automatically allocated by default.

Feature memory refers to the intermediate memory required during model execution, such as the input and output memory of intermediate nodes.

## Prototype

```c++
Status UpdateGraphRefreshableFeatureMemoryBase(uint32_t graph_id, const void *const memory, size_t size)
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| graph_id | Input | ID of the graph. |
| memory | Input | Refreshable Feature memory base address to set, except for Fixed. |
| size | Input | Size of the refreshable Feature memory to set, except for Fixed. |

## Return Value

| Name | Type | Description |
| --- | --- | --- |
| - | Status | **SUCCESS**: Set successfully.<br>**FAILED**: Failed to set. |

## Constraints

- Before calling this API, you must call the [CompileGraph](CompileGraph.md) API to compile the graph.
- After calling [CompileGraph](CompileGraph.md) to complete graph compilation, you can use the [GetCompiledGraphSummary](GetCompiledGraphSummary.md) API to check whether the Feature address is refreshable. Only graphs whose Feature address is refreshable support repeated calls to this API to refresh the Feature address.
- This API applies only to statically compiled graphs. You can use the **IsStatic** API in [GetCompiledGraphSummary](GetCompiledGraphSummary.md) to check whether a graph is statically compiled.
- If this API is used and the [RegisterExternalAllocator](RegisterExternalAllocator.md) API is also configured, the RegisterExternalAllocator API does not take effect.
- If the Fixed memory address of memory type MEMORY\_TYPE\_DEFAULT is set to nullptr and size is set to 0 through the [SetGraphFixedFeatureMemoryBase](SetGraphFixedFeatureMemoryBase.md) or [SetGraphFixedFeatureMemoryBaseWithType](SetGraphFixedFeatureMemoryBaseWithType.md) API, the Fixed memory feature is disabled, and this API can no longer be called in this scenario.
- This API cannot be used together with [UpdateGraphFeatureMemoryBase](UpdateGraphFeatureMemoryBase.md).
