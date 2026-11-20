# GetOutDataNodesAndPortIndexs

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:04:58.524Z pushedAt=2026-10-28T07:40:22.700Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/gnode.h\>
- Library file: libgraph.so

## Description

Obtains the peer operator corresponding to the specified output port of an operator and the input port number of the peer operator.

## Prototype

```c++
std::vector<std::pair<GNodePtr, int32_t>> GetOutDataNodesAndPortIndexs(const int32_t index) const
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| index | Input | Output port number of the operator. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | vector<std::pair<GNodePtr, int32_t >> | Peer operator corresponding to the specified output port of an operator and the input port number of the peer operator. |

## Constraints

None
