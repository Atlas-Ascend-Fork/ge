# infer\_shape

<!-- md-trans-meta sourceCommit=ff144808a5139d0516761fb6cf578e2e326a82f5 translatedAt=2026-10-15T03:48:52.538Z pushedAt=2026-10-16T10:53:07.592Z -->

## Applicable Products

Supported on all chips.

## Description

Used to perform [Shape](../graph/Shape/overview.md), [DataType](../DataType.md), and [Format](../Format.md) inference on the [Graph](../graph/Graph/overview.md) after a custom fusion pass constructs a replacement graph. The API first updates the [TensorDesc](../graph/TensorDesc/overview.md) of the input Data nodes of the replacement graph based on the original graph boundary information provided by `source`, and then performs full-graph inference on the replacement graph, updating the output TensorDesc of the operators in the graph in place. `source` supports [MatchResult](MatchResult/overview.md), [Node](../graph/Node/overview.md), and [SubgraphBoundary](SubgraphBoundary/overview.md).

## Prototype

```python
infer_shape(replacement: Graph, source: MatchResult | Node | SubgraphBoundary) -> None
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| replacement | Input | Replacement graph to be inferred. The API updates the **TensorDesc** of the input **Data** nodes in place, and updates the output **TensorDesc** of operators in the graph through full-graph inference. |
| source | Input | Source of the boundary information of the original graph. In the [PatternFusionPass](PatternFusionPass/overview.md) scenario, pass a **MatchResult**; in the [DecomposePass](DecomposePass/overview.md) scenario, pass a **Node** in the original graph; when modifying the graph directly using subgraph boundaries, pass a constructed **SubgraphBoundary**. |

## Return Value

Returns `None` on success.

- `TypeError`: The type of `replacement` or `source` is incorrect.
- `RuntimeError`: The underlying object handle is invalid, `replacement` is an empty graph, or an inference failure occurs. When an inference failure occurs, the exception message contains the replacement graph name, the source type, and the source name when available.

## Constraints

- When a pass is registered at the `BEFORE_INFER_SHAPE` stage of [PassStage](PassStage.md), there is no need to call this API. After the replacement graph is connected to the original graph, its operators participate in the graph-wide `InferShapePass` of the GE compilation process.
- When a pass is registered at the `AFTER_INFER_SHAPE` stage or a later stage, if the replacement graph introduces new operators whose output TensorDesc must be inferred based on the boundary inputs of the original graph, call this API before returning the replacement graph.
- If the replacement graph does not introduce operators that require inference, or if the input Data nodes and the TensorDesc of each operator have already been completely and correctly set during graph construction, there is no need to call this API. The caller must ensure that the TensorDesc of the replacement graph is correct.
- This API does not read or validate the current PassStage. Calling it at any stage immediately performs inference; it does not report an error or skip execution because of the `BEFORE_INFER_SHAPE` stage. It returns `None` on successful inference and throws `RuntimeError` on inference failure. After this API is explicitly called at the `BEFORE_INFER_SHAPE` stage, if the replacement graph is successfully connected to the original graph, its operators still participate in the graph-wide `InferShapePass` of the subsequent GE compilation process. If the exception is not caught, the current replacement callback is terminated.
- `replacement` must be of the Graph type and must not be an empty graph.
- `source` must be of the MatchResult, Node, or SubgraphBoundary type.
- The underlying object handles of `replacement` and `source` must be valid.
- The Data nodes in the replacement graph must have the `index` attribute, and `index` must be able to find the corresponding item in the boundary inputs provided by `source`.
- When `source` is a MatchResult, it can be used only during the callback execution of the current fusion pass. After the callback ends, the MatchResult becomes invalid.

## Examples

```python
from ge.es import GraphBuilder
from ge.passes import infer_shape


# PatternFusionPass replacement callback.
def replacement(self, match_result):
    builder = GraphBuilder()
    replacement_input = builder.create_input(0)
    replacement = builder.build_and_reset([replacement_input])
    infer_shape(replacement, match_result)
    return replacement
```
