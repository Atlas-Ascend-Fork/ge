# EsCreateVariable

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-27T16:20:23.263Z pushedAt=2026-10-28T13:21:45.740Z nl=2 -->

## Product Support

Full chip support.

## Header File/Library File

- Header file: \#include <ge/esb\_funcs.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Feature Description

Creates a variable node.

## Prototype

```c
EsCTensorHolder *EsCreateVariable(EsCGraphBuilder *graph, int32_t index, const char *name)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| graph | Input | Graph builder pointer. |
| index | Input | Node index. |
| name | Input | Node name. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | EsCTensorHolder * | Returns the created tensor holder pointer on success, or nullptr on failure. |

## Constraints

None

## Examples

```c
EsCGraphBuilder *graph = EsCreateGraphBuilder("test_graph");
auto variable = EsCreateVariable(graph, 0, "var0");
```
