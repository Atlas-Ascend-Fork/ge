# AttrType

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:42:12.788Z pushedAt=2026-10-28T07:40:22.371Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/compliant\_node\_builder.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Description

Sets the attribute type (required/optional).

## Prototype

```c++
IrAttrDefV2 &AttrType(IrAttrType ir_attr_type)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| ir_attr_type | Input | Attribute IR type enum value. For details, see [IrAttrType](../IrInputType.md). |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | IrAttrDefV2 & | Reference to the IR attribute definition class itself, supporting chained calls. |

## Constraints

None
