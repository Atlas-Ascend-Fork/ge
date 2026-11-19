# EsGetOwnerBuilder

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-11-17T15:40:07.965Z pushedAt=2026-11-18T06:41:26.179Z nl=2 -->

## Product Support

Full chip support.

## Header File/Library File

- Header file: \#include <ge/esb\_funcs.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Function Description

This API returns a snapshot of the EsCGraphBuilder instance, and ownership is not transferred.

## Prototype

```c
EsCGraphBuilder *EsGetOwnerBuilder(EsCTensorHolder *tensor)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| tensor | Input | Tensor holder pointer. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | EsCGraphBuilder * | Returns the graph builder pointer, or nullptr on failure. |

## Constraints

None

## Examples

```c
auto *tensor = EsCreateGraphInput(graph, 0);
auto *builder = EsGetOwnerBuilder(tensor);
```
