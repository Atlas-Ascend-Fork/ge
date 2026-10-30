# EsCTensorHolder Constructor and Destructor

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:55:39.101Z pushedAt=2026-10-28T07:40:22.429Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header/Library Files

- Header file: \#include <ge/es\_c\_tensor\_holder.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Description

Constructor and destructor of **EsCTensorHolder**.

## Prototype

```c++
EsCTensorHolder(EsCGraphBuilder &owner, const ge::GNode &producer, int32_t index)
~EsCTensorHolder()
EsCTensorHolder(EsCTensorHolder &&) noexcept
EsCTensorHolder &operator=(EsCTensorHolder &&) noexcept
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| owner | Input | Reference to **EsCGraphBuilder**, the graph builder. |
| producer | Input | Reference to **GNode**, the producer node. |
| index | Input | Output index. |

## Return Value

The **EsCTensorHolder** constructor returns an object of the **EsCTensorHolder** type.

## Constraints

None
