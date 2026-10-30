# GetInput

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:41:00.316Z pushedAt=2026-10-28T07:40:22.612Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/fusion/subgraph\_boundary.h\>
- Library file: libge\_compiler

## Description

Obtains a subgraph input by index.

## Prototype

```c++
Status GetInput(int64_t index, SubgraphInput &subgraph_input) const
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| index | Input | Input index. |
| subgraph_input | Output | Description of the subgraph input Tensor. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | Status | **SUCCESS**: Obtained successfully.<br>**FAILED**: Failed to obtain. |

## Constraints

None
