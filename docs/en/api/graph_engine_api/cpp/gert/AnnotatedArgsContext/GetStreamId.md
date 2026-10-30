# GetStreamId

<!-- md-trans-meta sourceCommit=f0a9fb067be0ab503b9c0f2dd50680654a1a1277 translatedAt=2026-10-28T07:58:08.380Z pushedAt=2026-10-28T08:50:45.429Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <exe\_graph/runtime/annotated\_args\_context.h\>
- Library file: liblowering.so

## Description

The graph framework supports [multi-stream concurrency optimization](https://www.hiascend.com/developer/techArticles/20240701-1?envFlag=1) at compile time. This API is used to obtain the logical stream ID of the node to which the current Context belongs.

## Prototype

```c++
uint32_t GetStreamId() const
```

## Parameters

None

## Return Value

Returns the logical primary stream ID of the current node on success; otherwise, returns UINT32_MAX.

## Constraints

None
