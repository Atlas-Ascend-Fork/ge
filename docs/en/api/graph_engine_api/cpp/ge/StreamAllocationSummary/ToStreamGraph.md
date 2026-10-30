# ToStreamGraph

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-26T13:33:49.313Z pushedAt=2026-10-28T07:42:19.863Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/ge\_graph\_compile\_summary.h\>
- Library file: libge\_compiler.so

## Description

Obtains the stream graph AscendString of the root graph and each subgraph, in DOT file format.

## Prototype

```c++
const std::map<AscendString, AscendString> &ToStreamGraph() const
```

## Parameters

None

## Return Value

Returns a Map whose key is the graph name and whose value is the DOT file representation of the stream graph.

## Constraints

None
