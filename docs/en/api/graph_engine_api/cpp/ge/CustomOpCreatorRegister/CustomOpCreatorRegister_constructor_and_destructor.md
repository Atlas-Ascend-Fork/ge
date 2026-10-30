# CustomOpCreatorRegister Constructor and Destructor

<!-- md-trans-meta sourceCommit=57819b78a98c428adc0bad0bffeeabc2858ca78b translatedAt=2026-10-16T12:25:28.410Z pushedAt=2026-10-28T07:40:22.299Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/custom\_op.h\>
- Library file: libgraph.so

## Description

Constructor and destructor of **CustomOpCreatorRegister**. A constructor that does not specify the **OpBackend** parameter registers to the Device backend by default.

## Prototype

```c++
CustomOpCreatorRegister(const AscendString &operator_type, const BaseOpCreator &op_creator)
CustomOpCreatorRegister(const AscendString &operator_type, OpBackend backend, const BaseOpCreator &op_creator)
CustomOpCreatorRegister(const AscendString &operator_type, const CustomOpCreateFunc op_creator)
CustomOpCreatorRegister(const AscendString &operator_type, OpBackend backend, const CustomOpCreateFunc op_creator)
~CustomOpCreatorRegister() = default
```
