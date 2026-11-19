# SetGraphConstMemoryBase

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-26T13:24:56.918Z pushedAt=2026-10-28T07:42:19.829Z nl=2 -->

## Header File/Library File

- Header file: \#include <ge/ge\_api.h\>
- Library file: libge\_runner.so

## Description

Sets the **Const** memory base address of a **Graph**.

The memory size is obtained from the [GetCompiledGraphSummary](GetCompiledGraphSummary.md)\>**GetConstMemorySize** API.

## Prototype

```c++
Status SetGraphConstMemoryBase(uint32_t graph_id, const void *const memory, size_t size)
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| graph_id | Input | ID of the Graph. |
| memory | Input | Base address of the Const memory to set. |
| size | Input | Size of the Const memory to set. |

## Return Value

| Name | Type | Description |
| --- | --- | --- |
| - | Status | **SUCCESS**: Set successfully.<br>**FAILED**: Failed to set. |

## Constraints

- Before calling this API, you must call [CompileGraph](CompileGraph.md) to compile the graph.
- Each **Graph** can be set only once, and refreshing is not supported.
- This API applies only to statically compiled graphs. You can use the **IsStatic** API in [GetCompiledGraphSummary](GetCompiledGraphSummary.md) to check whether a graph is statically compiled.
- If this API is used and the [RegisterExternalAllocator](RegisterExternalAllocator.md) API is also configured, the **RegisterExternalAllocator** API does not take effect.
