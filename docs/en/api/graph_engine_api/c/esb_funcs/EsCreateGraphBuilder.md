# EsCreateGraphBuilder

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-27T16:17:23.186Z pushedAt=2026-10-28T13:21:12.321Z nl=2 -->

## Product Support

Supported by all chips.

## Header File/Library File

- Header file: \#include <ge/esb\_funcs.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Feature Description

Creates a graph builder instance.

## Prototype

```c
EsCGraphBuilder *EsCreateGraphBuilder(const char *name)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| name | Input | Optional. Graph name, which defaults to "graph" if empty. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | EsCGraphBuilder * | Returns the graph builder pointer, or returns nullptr on failure. |

## Constraints

None

## Examples

```c
EsCGraphBuilder *builder = EsCreateGraphBuilder("graph_name");
```
