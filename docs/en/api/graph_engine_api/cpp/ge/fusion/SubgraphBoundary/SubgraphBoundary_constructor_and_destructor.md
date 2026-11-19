# SubgraphBoundary Constructor and Destructor

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:42:00.656Z pushedAt=2026-10-28T07:40:22.617Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/fusion/subgraph\_boundary.h\>
- Library file: libge\_compiler

## Description

Constructor and destructor of **SubgraphBoundary**.

## Prototype

```c++
SubgraphBoundary()
SubgraphBoundary(std::vector<SubgraphInput> inputs, std::vector<SubgraphOutput> outputs)
~SubgraphBoundary()
SubgraphBoundary(const SubgraphBoundary &other) noexcept
SubgraphBoundary &operator=(SubgraphBoundary &&other) noexcept
SubgraphBoundary &operator=(const SubgraphBoundary &other) noexcept
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| inputs | Input | All inputs of the subgraph boundary. |
| outputs | Output | All outputs of the subgraph boundary. |

## Return Value

None

## Constraints

None
