# AddInput

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:42:48.427Z pushedAt=2026-10-28T07:40:22.619Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/fusion/subgraph\_boundary.h\>
- Library file: libge\_compiler.so

## Description

Adds a boundary input to **SubgraphInput**.

## Prototype

```c++
Status AddInput(const NodeIo &node_input)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| node_input | Input | Node input. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | Status | **SUCCESS**: Success.<br>**FAILED**: Failure. |

## Constraints

None
