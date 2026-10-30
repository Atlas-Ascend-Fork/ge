# AllocateNextStreamId

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-26T13:33:56.111Z pushedAt=2026-10-28T07:42:19.865Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <register/register\_custom\_pass.h\>
- Library file: libregister.so

## Description

Applies for a new stream ID. Call this API when you want to apply for a new logical stream.

## Prototype

```c++
int64_t AllocateNextStreamId()
```

## Parameters

None

## Return Value

Returns the new stream ID.

## Constraints

None
