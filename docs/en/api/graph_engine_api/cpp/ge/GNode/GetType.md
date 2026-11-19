# GetType

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:07:19.996Z pushedAt=2026-10-28T07:40:22.708Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/gnode.h\>
- Library file: **libgraph.so**

## Description

Obtains the operator **type**.

## Prototype

```c++
graphStatus GetType(ge::AscendString &type) const
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| type | Output | Operator type. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | graphStatus | **GRAPH_SUCCESS(0)**: Success.<br>Other values: Failure. |

## Constraints

None
