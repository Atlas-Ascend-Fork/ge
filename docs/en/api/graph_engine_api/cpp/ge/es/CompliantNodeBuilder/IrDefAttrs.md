# IrDefAttrs

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:37:19.496Z pushedAt=2026-10-28T07:40:22.348Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/compliant\_node\_builder.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Description

Defines the IR attribute specification.

Note: This API is deprecated. Use the ABI-safe [IrDefAttrsV2](IrDefAttrsV2.md) API instead.

## Prototype

```c++
CompliantNodeBuilder &IrDefAttrs(std::vector<IrAttrDef> attr_ir_def)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| attr_ir_def | Input | Vector of attribute IR definitions. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | CompliantNodeBuilder & | Reference to the current builder object, supporting chained calls. |

## Constraints

None
