# GetFixedFeatureMemorySize

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:22:24.729Z pushedAt=2026-10-28T07:40:22.286Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/ge\_graph\_compile\_summary.h\>
- Library file: libge\_compiler.so

## Description

Obtains the total size of fixed feature memory after compilation (applicable to both static shape graphs and dynamic shape graphs).

## Prototype

```c++
Status GetFixedFeatureMemorySize(size_t &size) const
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| size | Output | Constant memory size. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | Status | - **SUCCESS**: The API is called successfully.<br>  - **FAILED**: The API call fails. |

## Constraints

None
