# AddOutput

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:40:05.116Z pushedAt=2026-10-28T07:40:22.607Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/fusion/subgraph\_boundary.h\>
- Library file: libge\_compiler

## Description

Adds an output to the subgraph boundary.

## Prototype

```c++
Status AddOutput(int64_t index, SubgraphOutput output)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| index | Input | Output index. |
| output | Input | Description of the output tensor. For details, see [SubgraphOutput](../SubgraphOutput/SubgraphOutput.md). |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | Status | **SUCCESS**: Success<br>**FAILED**: Failure |

## Constraints

None
