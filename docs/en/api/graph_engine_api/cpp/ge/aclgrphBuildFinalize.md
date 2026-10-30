# aclgrphBuildFinalize

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T11:03:08.476Z pushedAt=2026-10-28T07:40:22.103Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/ge\_ir\_build.h\>
- Library file: libge\_compiler.so

## Description

After the system completes model construction, this API releases resources.

## Prototype

```c++
void aclgrphBuildFinalize()
```

## Parameters

None

## Return Value

None

## Constraints

This API is called after [aclgrphBuildInitialize](aclgrphBuildInitialize.md) and can be called only once when the process exits.
