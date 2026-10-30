# EsSetOriginSymbolShape

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-27T16:27:35.539Z pushedAt=2026-10-28T13:29:02.018Z nl=2 -->

## Product Support

Full chip support.

## Header File/Library File

- Header file: \#include <ge/esb\_funcs.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Feature Description

Sets the symbolic representation of the Tensor shape. If not set, no symbolic information is used by default.

## Prototype

```c
uint32_t EsSetOriginSymbolShape(EsCTensorHolder *tensor, const char *const *shape, int64_t dim_num);
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| tensor | Input | Tensor holder pointer. |
| shape | Input | Pointer to the array of symbolic shape strings. |
| dim_num | Input | Number of symbolic shape strings. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | uint32_t | Returns 0 on success; otherwise, returns a non-zero value. |

## Constraints

None

## Examples

```c
/ 1. Create a graph builder (EsCGraphBuilder).
EsCGraphBuilder *builder = EsCreateGraphBuilder("graph_name");
// 2. Add the start node.
EsCTensorHolder *data0 = EsCreateInput(builder, 0); // Add the 0th input node, which defaults to a [] scalar.
const char* input_shape[] = {"a", "b", "c", "d"};
// Calculate the number of dimensions (dim_num).
int64_t dim_num = sizeof(input_shape) / sizeof(input_shape[0]); // The result is 4.
EsSetOriginSymbolShape(data0, input_shape, dim_num); // Set the symbolic expression of the shape of the 0th input node.
```
