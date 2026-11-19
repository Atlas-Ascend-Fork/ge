# EsCreateScalarInt64

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-11-17T15:36:28.099Z pushedAt=2026-11-18T06:40:54.082Z nl=2 -->

## Product Support

Full chip support.

## Header File/Library File

- Header file: \#include <ge/esb\_funcs.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Function Description

Creates a scalar constant of the int64 type.

## Prototype

```c
EsCTensorHolder *EsCreateScalarInt64(EsCGraphBuilder *graph, int64_t value)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| graph | Input | Graph builder pointer. |
| value | Input | Scalar value. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | EsCTensorHolder * | Returns the created Tensor holder pointer on success, or nullptr on failure. |

## Constraints

None

## Examples

```c
EsCGraphBuilder *graph = EsCreateGraphBuilder("test_graph");
auto *scalar64 = EsCreateScalarInt64(graph, 42);
```
