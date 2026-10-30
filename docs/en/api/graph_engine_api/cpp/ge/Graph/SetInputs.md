# SetInputs

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:24:05.889Z pushedAt=2026-10-28T07:40:22.769Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/graph.h\>
- Library file: libgraph.so

## Description

Sets the input operators in the **Graph**.

## Prototype

```c++
Graph &SetInputs(const std::vector<Operator> &inputs)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| inputs | Input | Input operators in the **Graph**. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | [Graph](Graph.md)& | Returns the caller itself. |

## Constraints

None
