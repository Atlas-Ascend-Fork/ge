# ListListTypeToPtrAndCounts

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:08:05.733Z pushedAt=2026-10-28T07:40:22.472Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/es\_graph\_builder.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Description

Converts the **ListListType** attribute input parameter into a **pair** containing **List of Ptrs** and **List inner size**.

## Prototype

```c++
template <typename T>
std::pair<std::vector<const T *>, std::vector<int64_t>> ListListTypeToPtrAndCounts(const std::vector<std::vector<T>> &list_list_type)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| T | Input | Type of **List of List**. |
| list_list_type | Input | **ListListType** attribute input parameter. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | std::pair<std::vector<const T *>, std::vector<int64_t>> | Pair containing **List of Ptrs** and **List inner size**. |

## Constraints

None
