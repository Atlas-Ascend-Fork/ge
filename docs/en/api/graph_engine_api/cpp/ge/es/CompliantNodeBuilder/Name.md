# Name

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:40:36.664Z pushedAt=2026-10-28T07:40:22.364Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/compliant\_node\_builder.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Description

Sets the node name.

## Prototype

```c++
CompliantNodeBuilder &Name(const char_t *name);
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| name | Input | Node name. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | CompliantNodeBuilder & | Reference to the current builder object, supporting chained calls. |

## Constraints

None
