# SetGraphOutput

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:55:05.151Z pushedAt=2026-10-28T07:40:22.426Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/es\_c\_graph\_builder.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Description

Sets the graph output.

## Prototype

```c++
ge::Status SetGraphOutput(EsCTensorHolder *tensor, int64_t output_index)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| tensor | Input | Tensor holder. |
| output_index | Input | Output index. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | ge::Status | Operation status. |

## Constraints

None
