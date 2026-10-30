# GetUsrStreamLabel

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:44:27.548Z pushedAt=2026-10-28T07:40:22.851Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/ge\_graph\_compile\_summary.h\>
- Library file: libge\_compiler.so

## Description

Obtains the user stream label corresponding to the current logical stream.

## Prototype

```c++
AscendString GetUsrStreamLabel() const
```

## Parameters

None

## Return Value

Returns the user stream label of the **AscendString** type. If the logical stream is not user-specified, an empty string is returned.

## Constraints

None
