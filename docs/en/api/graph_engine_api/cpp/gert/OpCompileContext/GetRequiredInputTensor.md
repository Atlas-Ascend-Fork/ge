# GetRequiredInputTensor

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-28T08:13:47.204Z pushedAt=2026-10-28T08:50:45.492Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <exe\_graph/runtime/op\_compile\_context.h\>
- Library file: liblowering.so

## Description

Obtains the input tensor pointer to the REQUIRED\_INPUT (required input) type based on the operator IR prototype definition, and locates it using the index in the IR prototype definition.

## Prototype

```c++
const Tensor *GetRequiredInputTensor(size_t ir_index) const
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| ir_index | Input | Index in the IR prototype definition. |

## Return Value

Returns the input tensor pointer, or a null pointer on exception.

## Constraints

None
