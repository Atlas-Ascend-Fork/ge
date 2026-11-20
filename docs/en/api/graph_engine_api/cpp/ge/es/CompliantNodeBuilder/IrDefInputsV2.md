# IrDefInputsV2

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:37:50.422Z pushedAt=2026-10-28T07:40:22.351Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/compliant\_node\_builder.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Description

Defines the ABI-safe IR input specification.

## Prototype

```c++
CompliantNodeBuilder &IrDefInputsV2(const IrInputDefV2 *input_ir_def, size_t input_ir_def_num)
CompliantNodeBuilder &IrDefInputsV2(std::initializer_list<IrInputDefV2> input_ir_def)
CompliantNodeBuilder &IrDefInputsV2(const std::vector<IrInputDefV2> &input_ir_def)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| input_ir_def | Input | Input IR definition array, which must remain valid during the call. |
| input_ir_def_num | Input | Number of input IR definitions. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | CompliantNodeBuilder & | Reference to the current builder object, supporting chained calls. |

## Constraints

None
