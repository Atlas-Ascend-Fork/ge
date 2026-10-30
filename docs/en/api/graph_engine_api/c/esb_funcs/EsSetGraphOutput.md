# EsSetGraphOutput

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-27T16:25:41.331Z pushedAt=2026-10-28T13:28:37.122Z nl=2 -->

## Product Support

Full chip support.

## Header File/Library File

- Header file: \#include <ge/esb\_funcs.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Feature Description

Sets the output of the graph.

## Prototype

```c
uint32_t EsSetGraphOutput(EsCTensorHolder *tensor, int64_t output_index)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| tensor | Input | Tensor to be output. |
| output_index | Input | Output index of the graph. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | uint32_t | 0 on success; otherwise, a failure value. |

## Constraints

None

## Examples

```c
EsCGraphBuilder *graph = EsCreateGraphBuilder("test_graph");
auto *tensor = EsCreateGraphInput(graph, 0);
uint32_t result = EsSetGraphOutput(tensor, 0);
```
