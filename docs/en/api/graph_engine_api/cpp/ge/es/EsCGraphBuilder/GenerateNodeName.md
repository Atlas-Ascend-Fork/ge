# GenerateNodeName

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:54:06.769Z pushedAt=2026-10-28T07:40:22.421Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/es\_c\_graph\_builder.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Description

Generates a node name.

## Prototype

```c++
ge::AscendString GenerateNodeName(const ge::char_t *node_type)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| node_type | Input | Node type. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | ge::AscendString | Generated node name string. |

## Constraints

None
