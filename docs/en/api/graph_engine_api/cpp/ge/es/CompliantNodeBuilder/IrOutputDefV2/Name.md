# Name

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:50:01.986Z pushedAt=2026-10-28T07:40:22.405Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/compliant\_node\_builder.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Description

Sets the output port name.

## Prototype

```c++
IrOutputDefV2 &Name(const char_t *name)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| name | Input | Output port name, which identifies the position of this output in the operator. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | IrOutputDefV2 & | Self-reference of the IR output definition class, which supports chained calls. |

## Constraints

None
