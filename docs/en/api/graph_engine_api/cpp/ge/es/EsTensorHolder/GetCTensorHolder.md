# GetCTensorHolder

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:11:26.881Z pushedAt=2026-10-28T07:40:22.484Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/es\_tensor\_holder.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Description

Obtains the pointer to the underlying C-language tensor holder. The lifetime of the returned value is managed by **EsGraphBuilder**, and the caller must not release it.

## Prototype

```c++
EsCTensorHolder *GetCTensorHolder() const
```

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | EsCTensorHolder * | Returns the pointer to the C-language tensor holder. |

## Constraints

None
