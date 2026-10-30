# Execute

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:29:53.078Z pushedAt=2026-10-28T07:40:22.321Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/custom\_op.h\>
- Library file: liblowering.so

## Description

Execution function of the Eager custom operator.

## Prototype

```c++
virtual graphStatus Execute(gert::EagerOpExecutionContext *ctx) = 0
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| ctx | Input | Execution context, through which you can obtain the input tensor, allocate output memory, allocate workspace, and so on. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | graphStatus | **GRAPH_SUCCESS(0)**: Success.<br>Other values: Failure. |

## Constraints

None
