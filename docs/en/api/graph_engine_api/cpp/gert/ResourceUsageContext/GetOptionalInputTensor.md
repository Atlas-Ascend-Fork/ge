# GetOptionalInputTensor

<!-- md-trans-meta sourceCommit=ff42e70ef4dd6e3da4c079832d1983d4ff97dea4 translatedAt=2026-10-28T08:15:44.560Z pushedAt=2026-10-28T08:50:45.498Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <exe\_graph/runtime/resource\_usage\_context.h\>
- Library file: liblowering.so

## Description

Obtains the pointer to the input tensor of the OPTIONAL\_INPUT (optional input) type based on the IR prototype definition, locates it using the index in the IR prototype definition, and reads the compile-time static shape, format, and dtype.

## Prototype

```c++
const Tensor *GetOptionalInputTensor(size_t ir_index) const
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| ir_index | Input | Index in the IR prototype definition. |

## Return Value

Returns the tensor pointer, or a null pointer if an exception occurs or the input is not instantiated.

## Constraints

None
