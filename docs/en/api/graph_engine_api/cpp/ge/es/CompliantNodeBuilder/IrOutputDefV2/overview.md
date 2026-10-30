# Overview

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:50:37.112Z pushedAt=2026-10-28T07:40:22.406Z nl=2 -->

An ABI-safe IR output definition class used to define the output specification of a node (output name, output type, and symbol identifier).

## Header File to Include

```c++
#include "compliant_node_builder.h"
```

## Public Member Functions

```c++
IrOutputDefV2()
IrOutputDefV2(const char_t *name, IrOutputType ir_output_type, const char_t *symbol_id)
~IrOutputDefV2()
IrOutputDefV2(const IrOutputDefV2 &other)
IrOutputDefV2 &operator=(const IrOutputDefV2 &other)
IrOutputDefV2(IrOutputDefV2 &&other) noexcept
IrOutputDefV2 &operator=(IrOutputDefV2 &&other) noexcept

IrOutputDefV2 &Name(const char_t *name)
IrOutputDefV2 &OutputType(IrOutputType ir_output_type)
IrOutputDefV2 &SymbolId(const char_t *symbol_id)

const char_t *GetName() const
IrOutputType GetOutputType() const
const char_t *GetSymbolId() const
```
