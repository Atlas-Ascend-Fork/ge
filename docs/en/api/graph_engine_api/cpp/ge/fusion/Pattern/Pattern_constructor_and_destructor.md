# Pattern Constructor and Destructor

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:33:19.008Z pushedAt=2026-10-28T07:40:22.578Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/fusion/pattern.h\>
- Library file: libge\_compiler.so

## Description

Uses **Graph** to define the matching pattern.

## Prototype

```c++
explicit Pattern(Graph &&pattern_graph)
~Pattern()
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| pattern_graph | Input | Pattern graph definition. After the Pattern is constructed, its lifecycle is managed by the Pattern object. |

## Return Value

None

## Constraints

None
