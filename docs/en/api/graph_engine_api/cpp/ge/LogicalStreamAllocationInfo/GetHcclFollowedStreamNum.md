# GetHcclFollowedStreamNum

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:43:19.829Z pushedAt=2026-10-28T07:40:22.847Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/ge\_graph\_compile\_summary.h\>
- Library file: libge\_compiler.so

## Description

Obtains the number of physical streams of the actual communication secondary stream corresponding to the current logical stream.

## Prototype

```c++
size_t GetHcclFollowedStreamNum() const
```

## Parameters

None

## Return Value

Number of physical streams of the communication secondary stream.

## Constraints

None
