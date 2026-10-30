# Overview

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:47:22.827Z pushedAt=2026-10-28T07:40:22.395Z nl=2 -->

ABI-safe IR input definition class.

## Header File to Include

```c++
#include "compliant_node_builder.h"
```

## Public Member Functions

```c++
IrInputDefV2()
IrInputDefV2(const char_t *name, IrInputType ir_input_type, const char_t *symbol_id)
~IrInputDefV2()
IrInputDefV2(const IrInputDefV2 &other)
IrInputDefV2 &operator=(const IrInputDefV2 &other)
IrInputDefV2(IrInputDefV2 &&other) noexcept
IrInputDefV2 &operator=(IrInputDefV2 &&other) noexcept
IrInputDefV2 &Name(const char_t *name)
IrInputDefV2 &InputType(IrInputType ir_input_type)
IrInputDefV2 &SymbolId(const char_t *symbol_id)
const char_t *GetName() const
IrInputType GetInputType() const
const char_t *GetSymbolId() const
const char_t *GetName() const
IrInputType GetInputType() const
const char_t *GetSymbolId() const
```
