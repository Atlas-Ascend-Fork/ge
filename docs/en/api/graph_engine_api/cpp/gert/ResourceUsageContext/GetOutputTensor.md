# GetOutputTensor

<!-- md-trans-meta sourceCommit=ff42e70ef4dd6e3da4c079832d1983d4ff97dea4 translatedAt=2026-10-28T08:16:10.617Z pushedAt=2026-10-28T08:50:45.500Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <exe\_graph/runtime/resource\_usage\_context.h\>
- Library file: liblowering.so

## Description

Obtains the output tensor pointer based on the output index, allowing you to read the static shape, format, and dtype determined at compile time.

## Prototype

```c++
const Tensor *GetOutputTensor(size_t index) const
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| index | Input | Output index. |

## Return Value

Returns the tensor pointer, or a null pointer on exception or out-of-range access.

## Constraints

None
