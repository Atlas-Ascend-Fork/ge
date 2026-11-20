# GetDynamicOutputTensor

<!-- md-trans-meta sourceCommit=ff42e70ef4dd6e3da4c079832d1983d4ff97dea4 translatedAt=2026-10-28T08:15:08.212Z pushedAt=2026-10-28T08:50:45.495Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <exe\_graph/runtime/resource\_usage\_context.h\>
- Library file: liblowering.so

## Description

Based on the operator IR prototype definition, obtains the output tensor pointer of the DYNAMIC\_OUTPUT (dynamic output) type. It requires two parameters, the IR prototype index and the relative index, to locate the specific output item after the dynamic output port is instantiated. It can read the static shape, format, and dtype at compile time.

## Prototype

```c++
const Tensor *GetDynamicOutputTensor(size_t ir_index, size_t relative_index) const
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| ir_index | Input | Index in the IR prototype definition, identifying which dynamic output port it is. |
| relative_index | Input | Relative index, identifying the specific output after the dynamic output is instantiated. For example, if a DYNAMIC\_OUTPUT instantiates three outputs, the valid range of relative\_index is [0, 2]. |

## Return Value

Returns the tensor pointer, or a null pointer on exception or out-of-range access.

## Constraints

None
