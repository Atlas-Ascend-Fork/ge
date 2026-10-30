# GetAttrDataType

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:42:57.674Z pushedAt=2026-10-28T07:40:22.374Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/compliant\_node\_builder.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Description

Obtains the data type of the attribute.

## Prototype

```c++
const char_t *GetAttrDataType() const
```

## Parameters

None

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | const char_t * | Pointer to the attribute data type string set through [AttrDataType](AttrDataType.md). |

## Constraints

None
