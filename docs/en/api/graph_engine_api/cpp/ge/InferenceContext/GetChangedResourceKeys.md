# GetChangedResourceKeys

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:28:49.020Z pushedAt=2026-10-28T07:40:22.789Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/inference\_context.h\>
- Library file: libgraph.so

## Description

Generally called by the framework.

After completing the deduction of write-type operators, you can call this API to obtain the changed resource identifiers.

## Prototype

```c++
const std::set<ge::AscendString>& GetChangedResourceKeys() const
```

## Parameters

None

## Return Value

Set of changed resource identifiers.

## Constraints

None
