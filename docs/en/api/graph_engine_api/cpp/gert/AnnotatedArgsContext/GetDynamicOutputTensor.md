# GetDynamicOutputTensor

<!-- md-trans-meta sourceCommit=f0a9fb067be0ab503b9c0f2dd50680654a1a1277 translatedAt=2026-10-28T07:55:55.504Z pushedAt=2026-10-28T08:50:45.422Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <exe\_graph/runtime/annotated\_args\_context.h\>
- Library file: liblowering.so

## Description

Obtains the output Tensor pointer to the `DYNAMIC_OUTPUT` type based on the operator IR prototype definition.

## Prototype

```c++
const Tensor *GetDynamicOutputTensor(size_t ir_index, size_t relative_index) const
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| ir_index | Input | Index in the IR prototype definition. |
| relative_index | Input | Relative index of the instantiated output. For example, if a DYNAMIC_OUTPUT is instantiated into three outputs, the valid range of relative_index is [0,2]. |

## Return Value

Returns the Tensor pointer, or a null pointer on failure.

## Constraints

None
