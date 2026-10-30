# IsStatic

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:24:42.375Z pushedAt=2026-10-28T07:40:22.295Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/ge\_graph\_compile\_summary.h\>
- Library file: libge\_compiler.so

## Description

Returns whether the graph is statically compiled. Only when the graph is statically compiled can the subsequent APIs for obtaining Summary information be used.

The **GetFixedFeatureMemorySize**/**GetAllFeatureMemoryTypeSize** APIs are exceptions.

## Prototype

```c++
bool IsStatic() const
```

## Parameters

None

## Return Value

Returns **true** if the graph is static; otherwise, returns **false**.

## Constraints

None
