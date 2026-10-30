# IsAssignedByStreamPass

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:44:55.408Z pushedAt=2026-10-28T07:40:22.853Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/ge\_graph\_compile\_summary.h\>
- Library file: libge\_compiler.so

## Description

Whether the current logical stream is allocated by the user-registered stream allocation Pass.

## Prototype

```c++
bool IsAssignedByStreamPass() const
```

## Parameters

None

## Return Value

bool type, true or false.

- **true**: Assigned by the user-registered stream allocation Pass.
- **false**: Comes from the stream split inside GE or the stream specified by the user stream label.

## Constraints

None
