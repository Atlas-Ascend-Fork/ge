# GetAllInputs

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:40:07.712Z pushedAt=2026-10-28T07:40:22.609Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/fusion/subgraph\_boundary.h\>
- Library file: libge\_compiler

## Description

Obtains the descriptions of all subgraph input tensors.

## Prototype

```c++
Status GetAllInputs(std::vector<SubgraphInput> &subgraph_input) const
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| subgraph_input | Output | Descriptions of all input tensors at the subgraph boundary. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | Status | **SUCCESS**: Succeeded<br>**FAILED**: Failed |

## Constraints

None
