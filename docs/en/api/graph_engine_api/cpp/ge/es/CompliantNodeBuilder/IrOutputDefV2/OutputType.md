# OutputType

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:50:16.050Z pushedAt=2026-10-28T07:40:22.408Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/compliant\_node\_builder.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Description

Sets the output type (required/dynamic).

## Prototype

```c++
IrOutputDefV2 &OutputType(IrOutputType ir_output_type)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| ir_output_type | Input | Output IR type enumeration value. For supported values, see [IrOutputType](../IrOutputType.md). |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | IrOutputDefV2 & | Self-reference of the IR output definition class, supporting chained calls. |

## Constraints

None
