# InputType

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:45:57.428Z pushedAt=2026-10-28T07:40:22.390Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/compliant\_node\_builder.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Description

Sets the type of the input definition, determining whether the input is required, optional, or dynamic.

## Prototype

```c++
IrInputDefV2 &InputType(IrInputType ir_input_type)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| ir_input_type | Input | Enum of the input definition. For supported values, see [IrInputType](../IrInputType.md). |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | IrInputDefV2 & | Reference to the IR input definition class itself, supporting chain calls. |

## Constraints

None
