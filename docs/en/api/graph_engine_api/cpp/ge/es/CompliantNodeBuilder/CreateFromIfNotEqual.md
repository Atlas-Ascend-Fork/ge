# CreateFromIfNotEqual

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:32:59.700Z pushedAt=2026-10-28T07:40:22.333Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/compliant\_node\_builder.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Description

If the value is not equal to the default value, an attribute value object is created.

## Prototype

```c++
template <typename T>
AttrValue CreateFromIfNotEqual(T &&value, typename std::decay<T>::type default_value)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| value | Input | Actual value. |
| default_value | Input | Default value. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | AttrValue | The created attribute value object. |

## Constraints

None
