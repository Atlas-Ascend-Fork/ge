# AddNodeByOp

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:13:05.278Z pushedAt=2026-10-28T07:40:22.728Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/graph.h\>
- Library file: libgraph.so

## Description

Adds a node to the graph.

## Prototype

```c++
GNode AddNodeByOp(const Operator &op)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| op | Input | Prototype op object of the node to be added. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | GNode | Returns the newly added node. |

## Constraints

At this point, the operator has no edges connected, and the node does not take effect. You need to explicitly add edge relationships.
