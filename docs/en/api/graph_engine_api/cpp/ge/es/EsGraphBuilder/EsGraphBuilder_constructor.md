# EsGraphBuilder Constructor

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:07:10.656Z pushedAt=2026-10-28T07:40:22.468Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/es\_graph\_builder.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Description

Constructor that creates a graph builder with the specified name.

## Prototype

```c++
explicit EsGraphBuilder(const char *name) : graph_builder_(EsCreateGraphBuilder(name), EsDestroyGraphBuilder) {}
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| name | Input | Name of the graph to be built. If **nullptr**, the default name "graph" is used. |

## Return Value

None

## Constraints

None

## Examples

```c++
// 1. Create a graph builder (EsGraphBuilder).
EsGraphBuilder builder("graph_name");
```
