# GetOutput

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:41:15.715Z pushedAt=2026-10-28T07:40:22.614Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/fusion/subgraph\_boundary.h\>
- Library file: libge\_compiler

## Description

Obtains the description of the subgraph output Tensor based on the output index.

## Prototype

```c++
Status GetOutput(int64_t index, SubgraphOutput &subgraph_output) const
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| index | Input | Output index. |
| subgraph_output | Output | Description of the output Tensor. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | Status | **SUCCESS**: Obtained successfully.<br>**FAILED**: Failed to obtain. |

## Constraints

None
