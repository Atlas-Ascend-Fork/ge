# Name

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:47:22.336Z pushedAt=2026-10-28T07:40:22.393Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/compliant\_node\_builder.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Description

Sets the name of the input definition, which is used to identify the input port name of a node.

## Prototype

```c++
IrInputDefV2 &Name(const char_t *name)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| name | Input | Input port name, which identifies the position of this input in the operator. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | IrInputDefV2 & | Self-reference of the IR input definition class, which supports chained calls. |

## Constraints

None
