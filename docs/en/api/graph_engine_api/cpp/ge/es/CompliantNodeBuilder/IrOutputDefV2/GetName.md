# GetName

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:48:10.639Z pushedAt=2026-10-28T07:40:22.398Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/compliant\_node\_builder.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Description

Obtains the output port name and returns the output port name previously set through [Name](./Name.md).

## Prototype

```c++
const char_t *GetName() const
```

## Parameters

None

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | const char_t * | Returns the string pointer to the output port name. The string content is the same as the value set through [Name](./Name.md). |

## Constraints

None
