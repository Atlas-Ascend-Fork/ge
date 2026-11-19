# GetInputTensor

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-28T08:12:10.299Z pushedAt=2026-10-28T08:50:45.484Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <exe\_graph/runtime/op\_compile\_context.h\>
- Library file: liblowering.so

## Description

Obtains the input Tensor pointer at the position corresponding to the input index.

## Prototype

```c++
const Tensor *GetInputTensor(size_t index) const
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| index | Input | Input index. |

## Return Value

Returns the input Tensor pointer, or a null pointer in case of an exception.

## Constraints

None
