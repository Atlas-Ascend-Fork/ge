# GetInDataNodesAndPortIndexs

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:01:50.381Z pushedAt=2026-10-28T07:40:22.688Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/gnode.h\>
- Library file: libgraph.so

## Description

Obtains the peer operator corresponding to the specified input port of the operator and the output port index of the peer operator.

## Prototype

```c++
std::pair<GNodePtr, int32_t> GetInDataNodesAndPortIndexs(const int32_t index) const
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| index | Input | Input port index of the operator. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | std::pair<GNodePtr, int32_t > | Peer input operator and the output port index of the peer input operator. An empty value indicates that there is no peer operator. |

## Constraints

None
