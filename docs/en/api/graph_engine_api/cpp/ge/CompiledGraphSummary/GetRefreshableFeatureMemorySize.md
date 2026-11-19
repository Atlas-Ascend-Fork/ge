# GetRefreshableFeatureMemorySize

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:23:57.027Z pushedAt=2026-10-28T07:40:22.293Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/ge\_graph\_compile\_summary.h\>
- Library file: libge\_compiler.so

## Description

Obtains the refreshable feature memory size after compilation, excluding input, output, and fixed memory.

## Prototype

```c++
Status GetRefreshableFeatureMemorySize(size_t &size) const
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| size | Output | Feature memory size. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | Status | - **SUCCESS**: The API is called successfully.<br>  - **FAILED**: The API call fails. |

## Constraints

None
