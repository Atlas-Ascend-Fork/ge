# aclgrphConvertToWeightRefreshableGraphs

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T11:05:06.221Z pushedAt=2026-10-28T07:40:22.112Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/ge\_ir\_build.h\>
- Library file: libge\_compiler.so

## Description

Converts the original graph into a set of weight-refreshable graphs by passing in an array of Const node names.

This API applies to weight update scenarios. For details, see [Weight Update](../../../../user_guides/graph_dev/more_features/weight_update.md).

## Prototype

```c++
graphStatus aclgrphConvertToWeightRefreshableGraphs(const ge::Graph &origin_graph, const std::vector<AscendString> &const_names, WeightRefreshableGraphs &weight_refreshable_graphs)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| origin_graph | Input | Original graph that requires weight update. |
| const_names | Input | Array of node names that require weight update. |
| weight_refreshable_graphs | Output | Weight-refreshable graph. This parameter is a struct that consists of three parts: weight initialization graph, weight update graph, and inference graph. For details, see the following structure example. |

```c++
struct WeightRefreshableGraphs {
  ge::Graph infer_graph;
  ge::Graph var_init_graph;
  ge::Graph var_update_graph;
};
```

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | graphStatus | GRAPH_SUCCESS(0): Success.<br>Other values: Failure. |

## Examples

For the complete example, see [Examples](aclgrphBundleSaveModel.md#examples).
