# InstanceOutputFormat

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:34:10.275Z pushedAt=2026-10-28T07:40:22.336Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/compliant\_node\_builder.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Description

Sets the output format.

## Prototype

```c++
CompliantNodeBuilder &InstanceOutputFormat(const char_t *name, ge::Format format)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| name | Input | Output name. |
| format | Input | Format. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | CompliantNodeBuilder & | Reference to the current builder object, supporting chained calls. |

## Constraints

None
