# IrDefAttrsV2

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:37:25.331Z pushedAt=2026-10-28T07:40:22.347Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/compliant\_node\_builder.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Description

Defines an ABI-safe IR attribute specification.

## Prototype

```c++
CompliantNodeBuilder &IrDefAttrsV2(const IrAttrDefV2 *attr_ir_def, size_t attr_ir_def_num)
CompliantNodeBuilder &IrDefAttrsV2(std::initializer_list<IrAttrDefV2> attr_ir_def)
CompliantNodeBuilder &IrDefAttrsV2(const std::vector<IrAttrDefV2> &attr_ir_def)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| attr_ir_def | Input | Array of attribute IR definitions, which must remain valid during the call. |
| attr_ir_def_num | Input | Number of attribute IR definitions. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | CompliantNodeBuilder & | Reference to the current builder object, supporting chained calls. |

## Constraints

None
