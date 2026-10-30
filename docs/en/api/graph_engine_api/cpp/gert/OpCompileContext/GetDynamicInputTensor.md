# GetDynamicInputTensor

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-28T08:11:33.440Z pushedAt=2026-10-28T08:50:45.483Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <exe\_graph/runtime/op\_compile\_context.h\>
- Library file: liblowering.so

## Description

Based on the operator IR prototype definition, this API obtains the input tensor pointer to the DYNAMIC\_INPUT (dynamic input) type. It requires two parameters, the IR prototype index and the relative index, to locate the specific input item after the dynamic input port is instantiated. This API is used for operator scenarios that process a variable number of inputs.

## Prototype

```c++
const Tensor *GetDynamicInputTensor(size_t ir_index, size_t relative_index) const
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| ir_index | Input | Index in the IR prototype definition, identifying which dynamic input port it is. |
| relative_index | Input | Relative index, identifying the specific input item after the dynamic input is instantiated. |

## Return Value

Returns the input tensor pointer, or a null pointer on exception.

## Constraints

None
