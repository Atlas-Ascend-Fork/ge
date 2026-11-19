# GetRequiredOutputTensor

<!-- md-trans-meta sourceCommit=ff42e70ef4dd6e3da4c079832d1983d4ff97dea4 translatedAt=2026-10-28T08:16:58.617Z pushedAt=2026-10-28T08:50:45.506Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <exe\_graph/runtime/resource\_usage\_context.h\>
- Library file: liblowering.so

## Description

Obtains the output tensor pointer to the REQUIRED\_OUTPUT (required output) type based on the operator IR prototype definition. This API maps the IR prototype index to the corresponding output instance. Unlike the flat instance index used by [`GetOutputTensor`](GetOutputTensor.md), it can read the static shape, format, and dtype at compile time.

## Prototype

```c++
const Tensor *GetRequiredOutputTensor(size_t ir_index) const
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| ir_index | Input | Index in the IR prototype definition. |

## Return Value

Tensor pointer. A null pointer is returned on exception or when the output is not instantiated.

## Constraints

None
