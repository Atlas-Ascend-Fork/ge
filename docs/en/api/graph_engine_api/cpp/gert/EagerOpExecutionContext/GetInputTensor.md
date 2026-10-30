# GetInputTensor

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-28T08:04:11.307Z pushedAt=2026-10-28T08:50:45.454Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <exe\_graph/runtime/eager\_op\_execution\_context.h\>
- Library file: liblowering.so

## Description

Obtains the input tensor pointer based on the input index.

## Prototype

```c++
const Tensor *GetInputTensor(size_t index) const
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| index | Input | Input index. |

## Return Value

Returns the tensor pointer, or a null pointer on exception.

## Constraints

None
