# EsSetShape

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-27T16:27:41.757Z pushedAt=2026-10-28T13:29:06.101Z nl=2 -->

## Product Support

Full chip support.

## Header File/Library File

- Header file: \#include <ge/esb\_funcs.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Feature Description

Sets the shape of the Tensor. If not set, it defaults to \[\], that is, a scalar.

## Prototype

```c
uint32_t EsSetShape(EsCTensorHolder *tensor, const int64_t *shape, int64_t dim_num)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| tensor | Input | Holder pointer. |
| shape | Input | Array pointer. |
| dim_num | Input | Number of dimensions in the array. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | uint32_t | Returns 0 on success; otherwise, returns a non-zero value. |

## Constraints

None

## Examples

```c
// 1. Create the graph builder (EsCGraphBuilder).
EsCGraphBuilder *builder = EsCreateGraphBuilder("graph_name");
// 2. Add the start node.
EsCTensorHolder *data0 = EsCreateInput(builder, 0); // Add the 0th input node, which defaults to a [] scalar.
int64_t input_shape[] = {1, 3, 224, 224};
// Calculate the number of dimensions (dim_num).
int64_t dim_num = sizeof(input_shape) / sizeof(input_shape[0]); // The result is 4.
EsSetShape(data0, input_shape, dim_num); // Set the shape of the 0th input node.
```
