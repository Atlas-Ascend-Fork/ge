# SetOutputs

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:24:53.726Z pushedAt=2026-10-28T07:40:22.772Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/graph.h\>
- Library file: libgraph.so

## Description

Sets the output operators associated with the **Graph**.

## Prototype

> [!NOTE]
>APIs whose data type is string will be deprecated in later versions. It is recommended to use APIs whose data type is not string.

```c++
Graph &SetOutputs(const std::vector<Operator>& outputs)
Graph &SetOutputs(const std::vector<std::pair<Operator, std::vector<size_t>>> &output_indexs)
Graph &SetOutputs(const std::vector<std::pair<ge::Operator, std::string> > &outputs)
Graph &SetOutputs(const std::vector<std::pair<ge::Operator, AscendString>> &outputs)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| outputs | Input | List of output nodes. If an output node contains multiple Outputs, all Outputs return results. The results are returned in the order of the operator list passed to this API, and multiple Outputs are sorted by port index in ascending order. |
| output_indexs | Input | Returns computation results according to the list of operators and their Outputs (described by port index, of the size_t type) specified by the user. The order is consistent with the input order of this API. |
| outputs | Input | Returns computation results according to the list of operators and their Outputs (described by port name, of the string type) specified by the user. The order is consistent with the input order of this API. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | [Graph](Graph.md)& | Returns the caller itself. |

## Constraints

This API must be called as the last step of the graph construction process. Deleting Outputs or Targets nodes after calling this API may cause exceptions.
