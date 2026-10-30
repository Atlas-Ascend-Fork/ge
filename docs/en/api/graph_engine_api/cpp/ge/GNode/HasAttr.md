# HasAttr

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:08:19.678Z pushedAt=2026-10-28T07:40:22.713Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/gnode.h\>
- Library file: libgraph.so

## Description

Queries whether the attribute exists.

## Prototype

```c++
bool HasAttr(const AscendString &name)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| name | Input | Specified attribute name. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | bool | **True** indicates that the attribute exists, and **False** indicates that it does not. |

## Constraints

None
