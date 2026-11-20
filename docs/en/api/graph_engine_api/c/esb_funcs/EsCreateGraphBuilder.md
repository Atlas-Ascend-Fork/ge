# EsCreateGraphBuilder

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-11-17T15:34:52.224Z pushedAt=2026-11-18T06:40:34.850Z nl=2 -->

## Product Support

Full chip support.

## Header File/Library File

- Header file: \#include <ge/esb\_funcs.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Function Description

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
| - | EsCGraphBuilder * | Returns the graph builder pointer, or nullptr on failure. |

## Constraints

None

## Examples

```c
EsCGraphBuilder *builder = EsCreateGraphBuilder("graph_name");
```
