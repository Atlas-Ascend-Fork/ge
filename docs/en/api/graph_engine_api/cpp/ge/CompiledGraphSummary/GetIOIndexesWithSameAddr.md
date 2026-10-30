# GetIOIndexesWithSameAddr

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:22:23.913Z pushedAt=2026-10-28T07:40:22.288Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/ge\_graph\_compile\_summary.h\>
- Library file: libge\_compiler.so

## Description

Obtains the input index and output index pairs of a **Graph** that have the same logical address after compilation.

## Prototype

```c++
Status GetIOIndexesWithSameAddr(std::vector<std::pair<uint32_t, uint32_t>> &io_indexes) const
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| io_indexes | Output | Input/output index pairs. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | Status | - **SUCCESS**: The API is called successfully.<br>  - **FAILED**: The API call fails. |

## Constraints

None
