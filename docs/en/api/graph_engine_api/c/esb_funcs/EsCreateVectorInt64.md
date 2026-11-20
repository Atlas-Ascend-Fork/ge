# EsCreateVectorInt64

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-11-17T15:38:50.380Z pushedAt=2026-11-18T06:41:13.634Z nl=2 -->

## Product Support

Full chip support.

## Header File/Library File

- Header file: \#include <ge/esb\_funcs.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Function Description

Creates an int64 vector constant.

## Prototype

```c
EsCTensorHolder *EsCreateVectorInt64(EsCGraphBuilder *graph, const int64_t *value, int64_t dim)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| graph | Input | Graph builder pointer. |
| value | Input | Vector data pointer. |
| dim | Input | Vector length. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | EsCTensorHolder * | Returns the created tensor holder pointer on success; otherwise, returns nullptr. |

## Constraints

None

## Examples

```c
EsCGraphBuilder *graph = EsCreateGraphBuilder("test_graph");
int64_t vec_64_data[] = {1, 2, 3, 4};
auto *vector64 = EsCreateVectorInt64(graph, vec_64_data, 4);
```
