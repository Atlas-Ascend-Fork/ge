# SubgraphOutput Constructor and Destructor

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:45:05.135Z pushedAt=2026-10-28T07:40:22.631Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/fusion/subgraph\_boundary.h\>
- Library file: libge\_compiler.so

## Description

Constructor and destructor of **SubgraphOutput**.

## Prototype

```c++
SubgraphOutput()
explicit SubgraphOutput(const NodeIo &node_output)
~SubgraphOutput()
SubgraphOutput(const SubgraphOutput &other) noexcept
SubgraphOutput &operator=(SubgraphOutput &&other) noexcept
SubgraphOutput &operator=(const SubgraphOutput &other) noexcept
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| node_output | Input | Node output. |

## Return Value

None

## Constraints

None
