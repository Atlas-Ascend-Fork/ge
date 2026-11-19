# AddOp

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:13:49.237Z pushedAt=2026-10-28T07:40:22.730Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/graph.h\>
- Library file: libgraph.so

## Description

User operator cache API. Through this API, operators without connection relationships can be cached in the graph for query and object retrieval.

## Prototype

```c++
graphStatus AddOp(const ge::Operator &op)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| op | Input | Operator to be added. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | graphStatus | **SUCCESS**: Operation succeeded.<br>**FAILED**: Operation failed. |

## Constraints

This API is optional. It is mainly used when users switch between multiple functions, where the operator object may be a local variable and thus cannot be obtained by multiple functions. In this case, the operator can be registered in the graph, and the previously created operator object can be obtained through the graph. However, after being registered in the graph, this operator object persists until the graph is destroyed.
