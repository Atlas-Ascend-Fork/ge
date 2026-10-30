# GetAttachedStreamIds

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:42:59.173Z pushedAt=2026-10-28T07:40:22.845Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/ge\_graph\_compile\_summary.h\>
- Library file: libge\_compiler.so

## Description

Obtains the IDs of the attached logical sub-streams of the current logical stream.

## Prototype

```c++
std::vector<int64_t> GetAttachedStreamIds() const
```

## Parameters

None

## Return Value

A vector of type int64\_t containing the attached logical sub-stream IDs.

## Constraints

None
