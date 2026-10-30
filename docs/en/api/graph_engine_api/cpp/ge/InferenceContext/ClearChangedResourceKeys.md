# ClearChangedResourceKeys

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:27:58.461Z pushedAt=2026-10-28T07:40:22.786Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/inference\_context.h\>
- Library file: libgraph.so

## Description

Generally called by the framework.

After changed resources trigger re-derivation, call this API to clear the changed resource keys stored in **inference\_context**.

## Prototype

```c++
void ClearChangedResourceKeys()
```

## Parameters

None

## Return Value

None

## Constraints

None
