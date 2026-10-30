# CreateFrom

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:32:10.624Z pushedAt=2026-10-28T07:40:22.329Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/compliant\_node\_builder.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Description

Helper function for creating an attribute value from any type.

## Prototype

```c++
template <typename T>
AttrValue CreateFrom(T &&t)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| t | Input | Input value. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | AttrValue | Created attribute value object. |

## Constraints

None
