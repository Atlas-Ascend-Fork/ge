# MallocWorkSpace

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-28T08:06:44.826Z pushedAt=2026-10-28T08:50:45.464Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <exe\_graph/runtime/eager\_op\_execution\_context.h\>
- Library file: liblowering.so

## Description

Allocates Workspace memory with Placement set to Device.

The memory is managed by the context constructor, so the API caller does not need to release it proactively.

## Prototype

```c++
void *MallocWorkSpace(size_t size)
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| size | Input | Memory size, in bytes. |

## Return Value

Returns the address pointer, or a null pointer on exception.

## Constraints

None
