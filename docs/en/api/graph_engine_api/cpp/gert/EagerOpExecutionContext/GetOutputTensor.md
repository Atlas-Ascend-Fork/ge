# GetOutputTensor

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-28T08:04:13.419Z pushedAt=2026-10-28T08:50:45.457Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <exe\_graph/runtime/eager\_op\_execution\_context.h\>
- Library file: liblowering.so

## Description

Obtains the output tensor pointer specified by index.

## Prototype

```c++
const Tensor *GetOutputTensor(size_t index) const
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| index | Input | Output index. |

## Return Value

Returns the output tensor pointer, or a null pointer on error.

## Constraints

None
