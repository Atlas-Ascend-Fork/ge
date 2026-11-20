# MallocOutputTensor

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-28T08:06:18.256Z pushedAt=2026-10-28T08:50:45.461Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <exe\_graph/runtime/eager\_op\_execution\_context.h\>
- Library file: liblowering.so

## Description

Allocates Device memory for an output Tensor and initializes its basic information.

The memory of the output Tensor is managed by the Context constructor. The API caller does not need to release it.

## Prototype

```c++
Tensor *MallocOutputTensor(size_t index, const StorageShape &shape, const StorageFormat &format, ge::DataType dtype)
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| index | Input | Output index. |
| shape | Input | Shape of the output tensor. |
| format | Input | Format of the output tensor. |
| dtype | Input | Data type of the output tensor. |

## Return Value

Returns a Tensor pointer, or a null pointer on error.

## Constraints

None
