# UpdateGraphRefreshableFeatureMemoryBase

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:57:07.016Z pushedAt=2026-10-28T07:40:22.676Z nl=2 -->

## Header File/Library File

- Header file: \#include <ge/ge\_api\_v2.h\>
- Library file: libge\_runner\_v2.so

## Description

This API is used to update the refreshable Feature memory base address other than Fixed memory.

Before calling this API, you must call the [CompileGraph](./CompileGraph.md) API to compile the graph. The memory size is obtained from the [GetCompiledGraphSummary](./GetCompiledGraphSummary.md)\>**GetRefreshableFeatureMemorySize** API.

If the graph contains Fixed memory and you have not called the [SetGraphFixedFeatureMemoryBaseWithType](./SetGraphFixedFeatureMemoryBaseWithType.md) API to set the Fixed Feature memory base address of the default memory type, Fixed memory is automatically allocated by default.

Feature memory refers to the intermediate memory required during model execution (for example, the input and output memory of intermediate nodes).

## Prototype

```c++
Status UpdateGraphRefreshableFeatureMemoryBase(uint32_t graph_id, const void *const memory, size_t size)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| graph_id | Input | ID of the graph. |
| memory | Input | Refreshable Feature memory base address other than Fixed memory to be set. |
| size | Input | Refreshable Feature memory size other than Fixed memory to be set. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | Status | SUCCESS: Set successfully.<br>FAILED: Failed to set. |

## Constraints

- After calling CompileGraph to complete graph compilation, you can call the GetCompiledGraphSummary API to obtain whether the Feature address is refreshable. Only graphs whose Feature address is refreshable support repeatedly calling this API to refresh the Feature address.
- This API applies only to statically compiled graphs. You can call the **IsStatic** API in **GetCompiledGraphSummary** to check whether the graph is statically compiled.
- If this API is used and the [RegisterExternalAllocator](./RegisterExternalAllocator.md) API is also configured, the RegisterExternalAllocator API does not take effect.
- If the Fixed memory address of memory type MEMORY\_TYPE\_DEFAULT is set to nullptr and size is set to 0 through [SetGraphConstMemoryBase](./SetGraphConstMemoryBase.md) or **SetGraphFixedFeatureMemoryBaseWithType**, the Fixed memory function is disabled, and this API can no longer be called in this scenario.
- This API cannot be used together with [UpdateGraphFeatureMemoryBase](./UpdateGraphFeatureMemoryBase.md).
