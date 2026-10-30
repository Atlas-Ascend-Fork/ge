# GetOptionalInputTensor

<!-- md-trans-meta sourceCommit=f0a9fb067be0ab503b9c0f2dd50680654a1a1277 translatedAt=2026-10-28T07:56:33.651Z pushedAt=2026-10-28T08:50:45.423Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <exe\_graph/runtime/annotated\_args\_context.h\>
- Library file: liblowering.so

## Description

Obtains the input tensor pointer to the OPTIONAL\_INPUT type based on the operator IR prototype definition.

## Prototype

```c++
const Tensor *GetOptionalInputTensor(size_t ir_index) const
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| ir_index | Input | Index in the IR prototype definition. |

## Return Value

Tensor pointer. A null pointer is returned when an exception occurs.

## Constraints

None
