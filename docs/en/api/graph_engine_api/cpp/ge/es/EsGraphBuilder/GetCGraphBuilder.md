# GetCGraphBuilder

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:07:59.688Z pushedAt=2026-10-28T07:40:22.471Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/es\_graph\_builder.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Description

Obtains the underlying C graph builder.

## Prototype

```c++
EsCGraphBuilder *GetCGraphBuilder() const
```

## Parameters

None

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | EsCGraphBuilder * | Returns the C graph builder pointer. The lifetime of the return value is managed by **EsGraphBuilder**. |

## Constraints

None
