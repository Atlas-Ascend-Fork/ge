# GetOptionalInputTensor

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-28T08:04:12.898Z pushedAt=2026-10-28T08:50:45.455Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <exe\_graph/runtime/eager\_op\_execution\_context.h\>
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
