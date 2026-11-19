# Constructors and Destructors of GraphBuffer

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:26:24.105Z pushedAt=2026-10-28T07:40:22.780Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/graph\_buffer.h\>
- Library file: libgraph\_base.so

## Description

**GraphBuffer** constructor and destructor.

## Prototype

```c++
GraphBuffer()
GraphBuffer(const GraphBuffer &) = delete
GraphBuffer &operator=(const GraphBuffer &) = delete
~GraphBuffer()
```

## Parameters

None

## Return Value

None

## Constraints

None
