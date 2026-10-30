# GetInputTensor

<!-- md-trans-meta sourceCommit=f0a9fb067be0ab503b9c0f2dd50680654a1a1277 translatedAt=2026-10-28T07:55:48.234Z pushedAt=2026-10-28T08:50:45.420Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <exe\_graph/runtime/annotated\_args\_context.h\>
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
