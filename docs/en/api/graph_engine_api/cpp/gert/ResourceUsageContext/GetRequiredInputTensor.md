# GetRequiredInputTensor

<!-- md-trans-meta sourceCommit=ff42e70ef4dd6e3da4c079832d1983d4ff97dea4 translatedAt=2026-10-28T08:16:25.952Z pushedAt=2026-10-28T08:50:45.501Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <exe\_graph/runtime/resource\_usage\_context.h\>
- Library file: liblowering.so

## Description

Based on the operator IR prototype definition, obtains the input tensor pointer to the REQUIRED\_INPUT (required input) type. It uses the index in the IR prototype definition for locating and can read the compile-time static shape, format, and dtype.

## Prototype

```c++
const Tensor *GetRequiredInputTensor(size_t ir_index) const
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| ir_index | Input | Index in the IR prototype definition. |

## Return Value

Returns the tensor pointer, or a null pointer on exception or when the input is not instantiated.

## Constraints

- Valid only during the `DeclareResourceUsage` callback. Do not cache it for use outside the callback.
