# DataTypesToEsCDataTypes

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:05:45.206Z pushedAt=2026-10-28T07:40:22.463Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/es\_graph\_builder.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Description

Converts a **ge::DataType** collection into a C\_DataType collection.

## Prototype

```c++
inline std::vector<C_DataType> DataTypesToEsCDataTypes(const std::vector<ge::DataType> &data_types)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| data_types | Input | Container of the **ge::DataType** type. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | std::vector<C_DataType> | Container of the **C_DataType** type. |

## Constraints

None
