# GetStreamNum

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:24:43.037Z pushedAt=2026-10-28T07:40:22.294Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/ge\_graph\_compile\_summary.h\>
- Library file: libge\_compiler.so

## Description

Obtains the number of streams used by the entire compilation graph.

## Prototype

```c++
Status GetStreamNum(size_t &num) const
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| **num** | Output | Number of streams used. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | **Status** | - **SUCCESS**: The API is called successfully.<br>  - **FAILED**: The API call fails. |

## Constraints

None
