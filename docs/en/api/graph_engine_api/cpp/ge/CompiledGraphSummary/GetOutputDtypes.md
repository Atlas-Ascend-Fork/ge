# GetOutputDtypes

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:23:08.816Z pushedAt=2026-10-28T07:40:22.290Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/ge\_graph\_compile\_summary.h\>
- Library file: libge\_compiler.so

## Description

Obtains the output Dtypes of the entire compilation graph.

## Prototype

```c++
Status GetOutputDtypes(std::vector<ge::DataType> &dtypes) const
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| dtypes | Output | Type in **ge::DataType**. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | Status | - **SUCCESS**: The API is called successfully.<br>  - **FAILED**: The API fails to be called. |

## Constraints

None
