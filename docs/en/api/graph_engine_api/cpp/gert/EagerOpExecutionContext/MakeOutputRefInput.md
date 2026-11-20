# MakeOutputRefInput

<!-- md-trans-meta sourceCommit=0d886336c529b55e75a4f7004f7a6e2ed7a35b2f translatedAt=2026-10-28T08:05:48.796Z pushedAt=2026-10-28T08:50:45.462Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <exe\_graph/runtime/eager\_op\_execution\_context.h\>
- Library file: liblowering.so

## Description

Specifies that the memory address of a given output references a given input.

## Prototype

```c++
Tensor *MakeOutputRefInput(size_t output_index, size_t input_index) const
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| output_index | Input | Output index. |
| input_index | Input | Input index. |

## Return Value

The output Tensor pointer corresponding to output_index.

## Constraints

The output parameter corresponding to output_index and the input parameter corresponding to input_index must have the same name in the operator IR prototype definition; otherwise, the API call fails.
