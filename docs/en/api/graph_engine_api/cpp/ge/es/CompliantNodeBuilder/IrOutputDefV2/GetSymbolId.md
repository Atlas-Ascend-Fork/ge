# GetSymbolId

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:48:35.714Z pushedAt=2026-10-28T07:40:22.401Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/compliant\_node\_builder.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Description

Obtains the symbol identifier of the output and returns the symbol ID previously set through [SymbolId](./SymbolId.md).

## Prototype

```c++
const char_t *GetSymbolId() const
```

## Parameters

None

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | const char_t * | Returns the pointer to the symbol identifier string of the output tensor. The string content is the same as the value set through [SymbolId](./SymbolId.md). |

## Constraints

None
