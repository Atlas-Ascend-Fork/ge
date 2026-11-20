# SetOriginSymbolShape

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:14:36.326Z pushedAt=2026-10-28T07:40:22.495Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/es\_tensor\_holder.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Description

Sets the original symbolic shape of the **TensorHolder**.

## Prototype

```c++
EsTensorHolder &SetOriginSymbolShape(const std::vector<const char *> &dims)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| dims | Input | Vector of symbolic dimension names. |

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
EsTensorHolder data0 = builder.CreateInput(0).SetOriginSymbolShape({"a", "b", "c", "d"}); // Set the symbolic shape of the output tensor of the input node.
```
