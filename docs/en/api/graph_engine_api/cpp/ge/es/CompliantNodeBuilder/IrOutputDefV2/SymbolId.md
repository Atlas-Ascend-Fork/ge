# SymbolId

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:51:23.140Z pushedAt=2026-10-28T07:40:22.409Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/compliant\_node\_builder.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Description

Sets the symbol identifier of the output for dynamic shape inference.

## Prototype

```c++
IrOutputDefV2 &SymbolId(const char_t *symbol_id)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| symbol_id | Input | Symbol identifier of the output tensor, establishing the shape association between the output and the input. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | IrOutputDefV2 & | Reference to the IR output definition class itself, supporting chained calls. |

## Constraints

None
