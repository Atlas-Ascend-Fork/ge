# EsCreateGraphInput

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-11-17T15:35:06.680Z pushedAt=2026-11-18T06:40:38.403Z nl=2 -->

## Product Support

Full chip support.

## Header File/Library File

- Header file: \#include <ge/esb\_funcs.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Function Description

Creates a Data input on a graph.

## Prototype

```c
EsCTensorHolder *EsCreateGraphInput(EsCGraphBuilder *graph, int64_t index)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| graph | Input | Graph builder pointer. |
| index | Input | Index of the graph input, counted from 0. The node is named input_{index}. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | EsCTensorHolder * | Returns the created tensor holder pointer on success; otherwise, returns nullptr. |

## Constraints

None

## Examples

```c
// 1. Create the graph builder (EsCGraphBuilder).
EsCGraphBuilder *builder = EsCreateGraphBuilder("graph_name");
// Create the first input node (index starts from 0).
EsCTensorHolder *input0 = EsCreateGraphInput(builder, 0);
// Create the second input node.
EsCTensorHolder *input1 = EsCreateGraphInput(builder, 1);
// Release after use.
EsDestroyGraphBuilder(builder);
```
