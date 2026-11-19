# BuildGraphAndReset

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:52:39.536Z pushedAt=2026-10-28T07:40:22.415Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/es\_c\_graph\_builder.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Description

Builds and returns the graph object.

## Prototype

```c++
std::unique_ptr<ge::Graph> BuildGraphAndReset()
```

## Parameters

None

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | std::unique_ptr\<ge::Graph> | The constructed graph object. |

## Constraints

None
