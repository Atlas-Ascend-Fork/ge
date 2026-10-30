# GetOwnerBuilder

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:56:39.565Z pushedAt=2026-10-28T07:40:22.433Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/es\_c\_tensor\_holder.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Description

Obtains the graph builder to which it belongs.

## Prototype

```c++
EsCGraphBuilder &GetOwnerBuilder()
```

## Parameters

None

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | EsCGraphBuilder & | Obtains the reference to the graph builder to which it belongs. |

## Constraints

None
