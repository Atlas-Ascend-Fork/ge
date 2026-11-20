# PaRemapped

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-26T13:19:37.184Z pushedAt=2026-10-28T07:42:19.814Z nl=2 -->

## Header File/Library File

- Header file: \#include <ge/ge\_api.h\>
- Library file: libge\_runner.so

## Description

Determines whether a memory segment can be quickly recovered when a UCE (Uncorrectable Error) occurs in the memory.

## Prototype

```c++
Status PaRemapped(const uint64_t va, const uint64_t new_pa, const uint64_t len)
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| va | Input | Virtual address where the UCE error occurs. |
| new_pa | Input | Physical address of the newly allocated memory. Reserved parameter. |
| len | Input | Size of the memory where the UCE error occurs. |

## Return Value

| Name | Type | Description |
| --- | --- | --- |
| - | Status | **SUCCESS**: The memory segment corresponding to va can be quickly recovered.<br>**FAILED**: The VA memory segment contains an unrecoverable address.<br>**PARAM_INVALID**: An unrecognized VA address segment exists. |

## Constraints

- Before calling this API, you must call the [CompileGraph](CompileGraph.md) API to compile the graph.
- Scenarios where Host scheduling exists in a Session are not supported. If a Host-scheduled graph exists in the Session, the API returns a failure.
- Absolute address segments in the allocation table of static shape graphs are not supported.
- This API applies only to statically compiled graphs. You can obtain whether a graph is statically compiled through the IsStatic API in the [GetCompiledGraphSummary](GetCompiledGraphSummary.md) API.
