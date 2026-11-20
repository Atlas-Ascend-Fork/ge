# GetInputTensor

<!-- md-trans-meta sourceCommit=ff42e70ef4dd6e3da4c079832d1983d4ff97dea4 translatedAt=2026-10-28T08:15:14.445Z pushedAt=2026-10-28T08:50:45.497Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <exe\_graph/runtime/resource\_usage\_context.h\>
- Library file: liblowering.so

## Description

Obtains the input tensor pointer based on the input index, and reads the static shape, format, and dtype at compile time.

## Prototype

```c++
const Tensor *GetInputTensor(size_t index) const
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| index | Input | Input index. |

## Return Value

Tensor pointer. Returns a null pointer on exception or out-of-range access.

## Constraints

None
