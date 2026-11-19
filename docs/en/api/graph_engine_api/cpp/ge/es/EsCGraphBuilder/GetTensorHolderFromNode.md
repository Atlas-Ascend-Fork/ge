# GetTensorHolderFromNode

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:54:23.629Z pushedAt=2026-10-28T07:40:22.424Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/es\_c\_graph\_builder.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Description

Obtains the tensor holder from a graph node.

## Prototype

```c++
EsCTensorHolder *GetTensorHolderFromNode(const ge::GNode &node, int32_t output_index)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| node | Input | Graph node. |
| output_index | Input | Output index. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | EsCTensorHolder * | Tensor holder pointer. |

## Constraints

None
