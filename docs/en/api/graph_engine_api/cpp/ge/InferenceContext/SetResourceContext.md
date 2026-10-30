# SetResourceContext

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:34:04.062Z pushedAt=2026-10-28T07:40:22.808Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/inference\_context.h\>
- Library file: libgraph.so

## Description

Sets the resource context object for the resource identified by **key** and hands it over to the resource context manager for management.

This API is generally called by write-type resource operators, such as stack push.

## Prototype

```c++
graphStatus SetResourceContext(const ge::AscendString &key, ResourceContext *resource_context)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| key | Input | Unique identifier of the resource. |
| resource_context | Input | Pointer to the resource context object. For details, see the return value description of [GetResourceContext](GetResourceContext.md). |

## Return Value

**graphStatus** type: **GRAPH\_SUCCESS** indicates success; **GRAPH\_FAILED** indicates failure.

## Constraints

If the resource context manager pointer is not passed in when creating **InferenceContext** using the [Create](Create.md) API, this API returns a failure.
