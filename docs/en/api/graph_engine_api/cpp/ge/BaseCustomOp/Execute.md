# Execute

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:18:08.396Z pushedAt=2026-10-28T07:40:22.273Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/custom\_op.h\>
- Library file: libgraph.so

## Description

Execution entry of a custom operator in Eager mode.

## Prototype

```c++
virtual graphStatus Execute(gert::EagerOpExecutionContext *ctx)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| ctx | Input | Execution context, through which you can obtain the input tensor, allocate output memory, allocate workspace, and so on. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | graphStatus | GRAPH_SUCCESS(0): Success<br>Other values: Failure |

## Constraints

None
