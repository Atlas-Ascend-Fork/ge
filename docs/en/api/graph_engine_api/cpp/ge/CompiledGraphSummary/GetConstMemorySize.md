# GetConstMemorySize

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:20:29.768Z pushedAt=2026-10-28T07:40:22.278Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/ge\_graph\_compile\_summary.h\>
- Library file: libge\_compiler.so

## Description

Obtains the size of the compiled Const memory.

## Prototype

```c++
Status GetConstMemorySize(size_t &size) const
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| size | Output | Size of the constant memory. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | Status | - **SUCCESS**: The API is called successfully.<br>  - **FAILED**: The API call fails. |

## Constraints

None
