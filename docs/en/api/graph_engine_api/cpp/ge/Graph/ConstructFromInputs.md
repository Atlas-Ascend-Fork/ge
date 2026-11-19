# ConstructFromInputs

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:14:29.320Z pushedAt=2026-10-28T07:40:22.732Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/graph.h\>
- Library file: libgraph.so

## Description

Supports generating a **Graph** object based on a user-constructed **Operator** object.

The functionality is the same as that of [SetInputs](SetInputs.md). **SetInputs** will be gradually deprecated, and this API should be used instead.

## Prototype

```c++
static GraphPtr ConstructFromInputs(const std::vector<Operator> &inputs, const AscendString &name)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| inputs | Input | Operator for the input of the entire graph. |
| name | Input | Name of the Graph. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | GraphPtr | Graph pointer, which returns the newly constructed graph. |

## Constraints

None

## Examples

```c++
GraphPtr graph;
graph =  Graph::ConstructFromInputs(inputs, graph_name);
graph->SetOutputs(outputs);
```
