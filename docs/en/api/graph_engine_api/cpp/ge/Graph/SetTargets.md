# SetTargets

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:25:07.654Z pushedAt=2026-10-28T07:40:22.773Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/graph.h\>
- Library file: libgraph.so

## Description

Sets the list of end nodes of the **Graph**.

The operators in this list must be executed, but their outputs are not returned to the user.

## Prototype

```c++
Graph &SetTargets(const std::vector<Operator> &targets)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| targets | Input | List of end nodes. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | [Graph](Graph.md)& | Returns the caller itself. |

## Constraints

This API must be called as the last step of the graph construction process. Deleting **Outputs** or **Targets** nodes after calling this API may cause exceptions.
