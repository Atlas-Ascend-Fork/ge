# GetAttrName

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:43:10.889Z pushedAt=2026-10-28T07:40:22.376Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/compliant\_node\_builder.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Description

Obtains the attribute name.

## Prototype

```c++
const char_t *GetAttrName() const
```

## Parameters

None

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | const char_t * | Returns the pointer to the attribute name string set through [AttrName](AttrName.md). |

## Constraints

None
