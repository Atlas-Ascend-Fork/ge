# GetOutputType

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:48:29.488Z pushedAt=2026-10-28T07:40:22.399Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/compliant\_node\_builder.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Description

Obtains the output type and returns the output IR type (required/optional/dynamic) previously set through [OutputType](OutputType.md).

## Prototype

```c++
IrOutputType GetOutputType() const
```

## Parameters

None

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | IrOutputType | For the supported enum values, see [OutputType](OutputType.md). |

## Constraints

None
