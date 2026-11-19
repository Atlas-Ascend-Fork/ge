# GetEventNum

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:20:30.955Z pushedAt=2026-10-28T07:40:22.281Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/ge\_graph\_compile\_summary.h\>
- Library file: libge\_compiler.so

## Description

Obtains the number of events used in the entire compiled graph.

## Prototype

```c++
Status GetEventNum(size_t &num) const
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| num | Output | Number of events used. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | Status | - SUCCESS: The API is called successfully.<br>  - FAILED: The API call fails. |

## Constraints

None
