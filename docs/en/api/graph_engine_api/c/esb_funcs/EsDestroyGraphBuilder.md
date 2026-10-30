# EsDestroyGraphBuilder

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-27T16:22:39.416Z pushedAt=2026-10-28T13:23:06.421Z nl=2 -->

## Product Support

Full chip support.

## Header File/Library File

- Header file: \#include <ge/esb\_funcs.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Feature Description

Releases graph builder resources.

## Prototype

```c
void EsDestroyGraphBuilder(EsCGraphBuilder *graph)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| graph | Input | Graph builder to be passed in. |

## Constraints

None

## Examples

```c
// Create a graph builder (EsCGraphBuilder).
EsCGraphBuilder *builder = EsCreateGraphBuilder("graph_name");
// Omit the graph construction code...
// Release the builder and the process resources it manages.
EsDestroyGraphBuilder(builder);
```
