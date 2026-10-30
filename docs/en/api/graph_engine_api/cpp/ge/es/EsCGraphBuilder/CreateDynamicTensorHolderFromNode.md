# CreateDynamicTensorHolderFromNode

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:52:55.928Z pushedAt=2026-10-28T07:40:22.417Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/es\_c\_graph\_builder.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Description

Obtains the dynamic output tensor from a graph node.

## Prototype

```c++
std::vector<EsCTensorHolder *> *CreateDynamicTensorHolderFromNode(const ge::GNode& node, int32_t start_idx, int32_t output_num)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| node | Input | Graph node. |
| start_idx | Input | Start point of the output index. |
| output_num | Input | Total number of dynamic outputs. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | std::vector<EsCTensorHolder *>* | Pointer to the dynamic output container holder. |

## Constraints

None
