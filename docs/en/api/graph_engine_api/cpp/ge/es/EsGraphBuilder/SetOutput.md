# SetOutput

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:09:44.984Z pushedAt=2026-10-28T07:40:22.476Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/es\_graph\_builder.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Description

Sets the output node of the graph.

## Prototype

```c++
static int64_t SetOutput(const EsTensorHolder &tensor, int64_t index);
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| tensor | Input | Output tensor holder. |
| index | Input | Output index, counted from 0. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | int64_t | Returns the operation result, 0 on success and a non-zero value on failure. |

## Constraints

None

## Examples

```c++
// 1. Create the graph builder (EsGraphBuilder).
EsGraphBuilder builder("graph_name");
// 2. Add two input nodes.
EsTensorHolder [data0, data1] = builder.CreateInputs<2>();
// 3. Add an intermediate node. In C++, common operators such as addition, subtraction, multiplication, and division are overloaded and can be used directly.
EsTensorHolder add = data0 + data1;
// 4. Set the graph output, where add is the 0th output tensor.
builder.SetOutput(add, 0);
```
