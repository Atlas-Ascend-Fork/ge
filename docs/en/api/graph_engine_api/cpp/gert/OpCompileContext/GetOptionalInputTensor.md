# GetOptionalInputTensor

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-28T08:13:06.641Z pushedAt=2026-10-28T08:50:45.487Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <exe\_graph/runtime/op\_compile\_context.h\>
- Library file: liblowering.so

## Description

Based on the operator IR prototype definition, obtains the input tensor pointer to the OPTIONAL\_INPUT (optional input) type. Use this API when the operator has optional input parameters.

## Prototype

```c++
const Tensor *GetOptionalInputTensor(size_t ir_index) const
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| ir_index | Input | Index in the IR prototype definition. |

## Return Value

Returns the input tensor pointer, or a null pointer on exception.

## Constraints

None
