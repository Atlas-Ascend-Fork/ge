# GetReliedOnResourceKeys

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:30:24.252Z pushedAt=2026-10-28T07:40:22.796Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/inference\_context.h\>
- Library file: libgraph.so

## Description

Generally called by the framework.

After the inference of read-type operators is complete, you can call this API to obtain the dependent resource keys.

## Prototype

```c++
const std::set<ge::AscendString>& GetReliedOnResourceKeys() const
```

## Parameters

None

## Return Value

The set of dependent resource keys.

## Constraints

None
