# GetAllOutputs

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:40:32.977Z pushedAt=2026-10-28T07:40:22.611Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/fusion/subgraph\_boundary.h\>
- Library file: libge\_compiler

## Description

Obtains the descriptions of all output Tensors of the subgraph.

## Prototype

```c++
Status GetAllOutputs(std::vector<SubgraphOutput> &subgraph_outputs) const
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| subgraph_outputs | Output | Descriptions of all output Tensors of the subgraph. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | Status | **SUCCESS**: Obtained successfully.<br>**FAILED**: Failed to obtain. |

## Constraints

None
