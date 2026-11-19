# GetAllLogicalStreamInfos

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-26T13:32:46.226Z pushedAt=2026-10-28T07:42:19.857Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/ge\_graph\_compile\_summary.h\>
- Library file: libge\_compiler.so

## Description

Obtains the logical stream information of the root graph and subgraphs.

## Prototype

```c++
const std::map<AscendString, std::vector<LogicalStreamAllocationInfo>> &GetAllLogicalStreamInfos() const
```

## Parameters

None

## Return Value

Returns a Map where the key is the graph name and the value is the logical stream allocation information.

## Constraints

None
