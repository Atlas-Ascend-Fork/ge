# SetGraphConstMemoryBase

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:55:01.415Z pushedAt=2026-10-28T07:40:22.667Z nl=2 -->

## Header File/Library File

- Header file: \#include <ge/ge\_api\_v2.h\>
- Library file: libge\_runner\_v2.so

## Description

Sets the Const memory base address of a **Graph**.

Before calling this API, you must first call [CompileGraph](./CompileGraph.md) to compile the graph. The memory size is obtained from the [GetCompiledGraphSummary](./GetCompiledGraphSummary.md)\>**GetConstMemorySize** API.

## Prototype

```c++
Status SetGraphConstMemoryBase(uint32_t graph_id, const void *const memory, size_t size)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| graph_id | Input | ID of the **Graph**. |
| memory | Input | Const memory base address to be set. |
| size | Input | Const memory size to be set. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | Status | **SUCCESS**: Set successfully.<br>**FAILED**: Failed to set. |

## Constraints

- Each **Graph** can be set only once, and refreshing is not supported.
- This API applies only to statically compiled graphs. You can call the **IsStatic** API in the **GetCompiledGraphSummary** API to check whether the graph is statically compiled.
- If this API is used and the [RegisterExternalAllocator](./RegisterExternalAllocator.md) API is also configured, the **RegisterExternalAllocator** API does not take effect.
