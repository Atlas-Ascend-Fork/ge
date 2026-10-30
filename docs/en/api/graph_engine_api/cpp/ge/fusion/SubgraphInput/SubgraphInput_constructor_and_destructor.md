# SubgraphInput Constructor and Destructor

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:43:35.129Z pushedAt=2026-10-28T07:40:22.624Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/fusion/subgraph\_boundary.h\>
- Library file: libge\_compiler.so

## Description

Constructor and destructor of **SubgraphInput**.

## Prototype

```c++
SubgraphInput()
explicit SubgraphInput(std::vector<NodeIo> node_inputs)
~SubgraphInput()
SubgraphInput(const SubgraphInput &other) noexcept
SubgraphInput &operator=(SubgraphInput &&other) noexcept
SubgraphInput &operator=(const SubgraphInput &other) noexcept
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| node_inputs | Input | Node input. |

## Return Value

None

## Constraints

None
