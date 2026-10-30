# IrDefOutputs

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:38:35.457Z pushedAt=2026-10-28T07:40:22.353Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/compliant\_node\_builder.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Description

Defines the IR output specification.

Note: This API is deprecated. It is recommended to use the ABI-safe [IrDefOutputsV2](IrDefOutputsV2.md) API.

## Prototype

```c++
CompliantNodeBuilder &IrDefOutputs(std::vector<IrOutputDef> output_ir_def)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| output_ir_def | Input | Output IR definition vector. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | CompliantNodeBuilder & | Reference to the current builder object, supporting chained calls. |

## Constraints

None
