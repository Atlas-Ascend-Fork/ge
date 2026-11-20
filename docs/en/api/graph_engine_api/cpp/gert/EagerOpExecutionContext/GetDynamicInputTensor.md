# GetDynamicInputTensor

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-28T08:03:35.203Z pushedAt=2026-10-28T08:50:45.452Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <exe\_graph/runtime/eager\_op\_execution\_context.h\>
- Library file: liblowering.so

## Description

Obtains the input tensor pointer to the DYNAMIC\_INPUT type based on the operator IR prototype definition.

## Prototype

```c++
const Tensor *GetDynamicInputTensor(size_t ir_index, size_t relative_index) const
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| ir_index | Input | Index in the IR prototype definition. |
| relative_index | Input | Relative index after the input is instantiated. For example, if a DYNAMIC_INPUT is instantiated into three inputs, the valid range of relative_index is [0,2]. |

## Return Value

Returns the tensor pointer, or a null pointer on exception.

## Constraints

None
