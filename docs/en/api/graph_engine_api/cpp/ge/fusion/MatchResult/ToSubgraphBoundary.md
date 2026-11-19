# ToSubgraphBoundary

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:30:39.291Z pushedAt=2026-10-28T07:40:22.563Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/fusion/match\_result.h\>
- Library file: libge\_compiler.so

## Description

Obtains the subgraph boundary corresponding to the match result.

## Prototype

```c++
std::unique_ptr<SubgraphBoundary> ToSubgraphBoundary() const
```

## Parameters

None

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | std::unique_ptr\<SubgraphBoundary> | A subgraph boundary definition. For details, see [SubgraphBoundary](../SubgraphBoundary/SubgraphBoundary.md). |

## Constraints

None
