# Compile

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:18:55.154Z pushedAt=2026-10-28T07:40:22.277Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/custom\_op.h\>
- Library file: libgraph.so

## Description

Compilation callback API of a custom operator. It is invoked when the operator enters the GE graph construction and compilation process to complete compilation-related processing of the operator (such as generating compilation artifacts and preparing the kernel), and obtains compilation-time information through the compilation context.

## Prototype

```c++
virtual graphStatus Compile(gert::OpCompileContext *ctx) = 0
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| ctx | Input | Operator compilation context. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | graphStatus | **GRAPH_SUCCESS(0)**: Success.<br>Other values: Failure. |

## Constraints

None
