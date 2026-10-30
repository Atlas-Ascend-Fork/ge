# GetDynamicInputTensor

<!-- md-trans-meta sourceCommit=f0a9fb067be0ab503b9c0f2dd50680654a1a1277 translatedAt=2026-10-28T07:55:17.675Z pushedAt=2026-10-28T08:50:45.418Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <exe\_graph/runtime/annotated\_args\_context.h\>
- Library file: liblowering.so

## Description

Obtains the input Tensor pointer to the DYNAMIC\_INPUT type based on the operator IR prototype definition.

## Prototype

```c++
const Tensor *GetDynamicInputTensor(size_t ir_index, size_t relative_index) const
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| ir_index | Input | Index in the IR prototype definition. |
| relative_index | Input | Relative index of this input after instantiation. For example, if a DYNAMIC_INPUT is instantiated into three inputs, the valid range of relative_index is [0,2]. |

## Return Value

Tensor pointer. Returns a null pointer on exception.

## Constraints

None
