# SetPassName

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:27:53.156Z pushedAt=2026-10-28T07:40:22.310Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <register/register\_custom\_pass.h\>
- Library file: **libregister.so**

## Description

Sets the name of the custom Pass.

## Prototype

```c++
void SetPassName(const AscendString &pass_name)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| pass_name | Input | Name of the Pass to be set. |

## Return Value

None

## Constraints

None
