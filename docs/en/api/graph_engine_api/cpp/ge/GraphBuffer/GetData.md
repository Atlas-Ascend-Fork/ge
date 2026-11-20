# GetData

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:25:35.365Z pushedAt=2026-10-28T07:40:22.775Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/graph\_buffer.h\>
- Library file: libgraph\_base.so

## Description

Obtains the memory start address of the serialized Graph.

## Prototype

```c++
const std::uint8_t *GetData() const
```

## Parameters

None

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | std::uint8_t* | Memory start address of the serialized Graph. |

## Constraints

None
