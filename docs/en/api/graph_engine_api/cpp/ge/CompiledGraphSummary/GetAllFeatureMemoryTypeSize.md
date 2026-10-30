# GetAllFeatureMemoryTypeSize

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:19:45.847Z pushedAt=2026-10-28T07:40:22.279Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/ge\_graph\_compile\_summary.h\>
- Library file: libge\_compiler.so

## Description

Obtains the Feature memory sizes of all memory types after compilation. Currently, only the Fixed memory size is included (applicable to both static shape graphs and dynamic shape graphs).

## Prototype

```c++
std::vector<FeatureMemoryPtr> GetAllFeatureMemoryTypeSize() const
```

## Parameters

None

## Return Value

The **shared_ptr** of [FeatureMemory](../FeatureMemory/FeatureMemory.md) in the graph compilation result.

## Constraints

None
