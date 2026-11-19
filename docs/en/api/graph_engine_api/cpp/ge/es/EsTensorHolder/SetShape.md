# SetShape

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:14:50.986Z pushedAt=2026-10-28T07:40:22.496Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/es\_tensor\_holder.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Description

Sets the shape of the **TensorHolder**.

## Prototype

```c++
EsTensorHolder &SetShape(const std::vector<int64_t> &dims)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| dims | Input | Tensor dimension vector. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | EsTensorHolder & | Returns a reference to the current object, supporting chained calls. |

## Constraints

None

## Examples

```c++
// 1. Create a graph builder (EsGraphBuilder).
EsGraphBuilder builder("graph_name");
// 2. Add an input node.
EsTensorHolder data0 = builder.CreateInput(0).SetShape({1, 3, 224, 224}); // Set the shape of the output tensor of the input node.
```
