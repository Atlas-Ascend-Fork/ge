# EsBuildGraphAndReset

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-27T16:13:53.499Z pushedAt=2026-10-28T13:20:12.748Z nl=2 -->

## Product Support

Full chip support.

## Header File/Library File

- Header file: \#include <ge/esb\_funcs.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Feature Description

This API returns a pointer to a ge::Graph instance, and ownership is transferred to the caller.

## Prototype

```c
EsCGraph *EsBuildGraphAndReset(EsCGraphBuilder *graph)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| graph | Input | Graph builder pointer. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | EsCGraph * | Returns the graph pointer on success, or nullptr on failure. Ownership is transferred to the caller, and the graph builder is no longer available after the call. |

## Constraints

None

## Examples

```c
EsCGraphBuilder *graph = EsCreateGraphBuilder("test_graph");
auto *tensor = EsCreateGraphInput(graph, 0);
EsCGraph *build_graph = EsBuildGraphAndReset(graph);
```
