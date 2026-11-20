# GetInputConstData

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:02:43.579Z pushedAt=2026-10-28T07:40:22.690Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/gnode.h\>
- Library file: libgraph.so

## Description

Obtains the value of the input that is a Const node.

If the input of the operator is a Const node, the value of the Const node is returned; otherwise, a failure is returned. It supports obtaining, across subgraphs, whether the input is a Const node and the value under the Const node.

## Prototype

```c++
graphStatus GetInputConstData(const int32_t index, Tensor &data) const
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| index | Input | Input port number of the operator. |
| data | Output | Value of the input Const node. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | graphStatus | **GRAPH_SUCCESS** indicates success, **GRAPH_NODE_WITHOUT_CONST_INPUT** indicates that the input is not a Const node, and other values indicate failure. |

## Constraints

None
