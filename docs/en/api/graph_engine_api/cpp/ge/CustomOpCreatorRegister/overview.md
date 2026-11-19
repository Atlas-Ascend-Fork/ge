# Overview

<!-- md-trans-meta sourceCommit=57819b78a98c428adc0bad0bffeeabc2858ca78b translatedAt=2026-10-16T12:26:09.993Z pushedAt=2026-10-28T07:40:22.302Z nl=2 -->

Custom operator registrar, used to register custom operators.

## Header File to Include

```c++
#include <graph/custom_op.h>
```

## Public Member Functions

```c++
CustomOpCreatorRegister(const AscendString &operator_type, const BaseOpCreator &op_creator)
CustomOpCreatorRegister(const AscendString &operator_type, OpBackend backend, const BaseOpCreator &op_creator)
CustomOpCreatorRegister(const AscendString &operator_type, const CustomOpCreateFunc op_creator)
CustomOpCreatorRegister(const AscendString &operator_type, OpBackend backend, const CustomOpCreateFunc op_creator)
~CustomOpCreatorRegister() = default
```
