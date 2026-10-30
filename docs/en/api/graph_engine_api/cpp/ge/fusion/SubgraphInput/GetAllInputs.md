# GetAllInputs

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:42:50.074Z pushedAt=2026-10-28T07:40:22.620Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/fusion/subgraph\_boundary.h\>
- Library file: libge\_compiler.so

## Description

Obtains all boundary inputs corresponding to **SubgraphInput**.

## Prototype

```c++
std::vector<NodeIo> GetAllInputs() const
```

## Parameters

None

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | std::vector\<NodeIo> | **NodeIo** represents an input index of a **Node**. |

## Constraints

None
