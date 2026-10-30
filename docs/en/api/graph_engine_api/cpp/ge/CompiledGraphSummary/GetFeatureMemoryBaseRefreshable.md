# GetFeatureMemoryBaseRefreshable

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:21:41.092Z pushedAt=2026-10-28T07:40:22.285Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/ge\_graph\_compile\_summary.h\>
- Library file: libge\_compiler.so

## Description

Obtains whether the graph supports Feature memory base refresh.

## Prototype

```c++
Status GetFeatureMemoryBaseRefreshable (bool &v) const
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| v | Output | Whether the memory base can be refreshed. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | Status | - **SUCCESS**: The API is called successfully.<br>  - **FAILED**: The API call fails. |

## Constraints

None
