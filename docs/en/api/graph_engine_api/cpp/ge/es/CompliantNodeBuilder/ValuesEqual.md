# ValuesEqual

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:40:59.882Z pushedAt=2026-10-28T07:40:22.366Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/compliant\_node\_builder.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Description

Compares whether two values are equal.

## Prototype

```c++
template <typename T>
bool ValuesEqual(const T &a, const T &b)
bool ValuesEqual(const std::vector<T> &a, const std::vector<T> &b)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| a | Input | The first value. |
| b | Input | The second value. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | bool | Returns **true** if the values are equal; otherwise, returns **false**. |

## Constraints

None
