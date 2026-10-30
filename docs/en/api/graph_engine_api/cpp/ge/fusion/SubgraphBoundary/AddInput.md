# AddInput

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:39:51.746Z pushedAt=2026-10-28T07:40:22.605Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/fusion/subgraph\_boundary.h\>
- Library file: libge\_compiler

## Description

Adds an input to the subgraph boundary.

## Prototype

```c++
Status AddInput(int64_t index, SubgraphInput input)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| index | Input | Input index. |
| input | Input | Description of the input tensor. For details, see [SubgraphInput](../SubgraphInput/SubgraphInput.md). |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | Status | **SUCCESS**: Success<br>**FAILED**: Failure |

## Constraints

None
