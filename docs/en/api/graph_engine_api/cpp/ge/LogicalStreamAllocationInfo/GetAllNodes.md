# GetAllNodes

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:42:32.810Z pushedAt=2026-10-28T07:40:22.844Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/ge\_graph\_compile\_summary.h\>
- Library file: libge\_compiler.so

## Description

Obtains all **GNode**s on the current logical flow.

## Prototype

```c++
const std::vector<GNode> &GetAllNodes() const
```

## Parameters

None

## Return Value

Vector of **GNode**.

## Constraints

None
