# SymbolId

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:47:33.581Z pushedAt=2026-10-28T07:40:22.396Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/compliant\_node\_builder.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Description

Sets the symbol identifier of the input definition, which is used for dynamic shape inference or symbolic expression to establish the symbolic association between inputs and outputs.

## Prototype

```c++
IrInputDefV2 &SymbolId(const char_t *symbol_id)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| symbol_id | Input | Symbol identifier of the input tensor, used for:<br><br>  - Dynamic shape scenarios: identifies dynamic dimensions (such as batch_size, height, and width).<br>  - Symbolic inference: establishes the symbolic relationship between inputs and outputs (similar to variable symbols in a mathematical formula).<br>  - Shape association: when multiple tensors share the same symbol, the shape relationship between them can be inferred. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | IrInputDefV2 & | Self-reference of the IR input definition class, supporting chained calls. |

## Constraints

None
