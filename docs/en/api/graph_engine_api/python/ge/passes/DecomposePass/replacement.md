# replacement

<!-- md-trans-meta sourceCommit=57e742592fb0b093fa4f5d82c393c97b374a8bfd translatedAt=2026-10-15T03:52:07.672Z pushedAt=2026-10-16T10:53:07.604Z -->

## Applicable Products

Supported on all chips.

## Description

Generates a decomposed subgraph.

## Prototype

```python
replacement(self, node: Node) -> Graph
replacement(self, node: Node, context: PassContext) -> Graph
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| node | Input | Node to be decomposed, of type **ge.graph.Node**. |
| context | Input | Current compilation-phase **PassContext**, valid only within the current callback stack. It can read compilation options or set error information. |

## Return Value

| Type | Description |
| --- | --- |
| Graph | Returns the decomposed subgraph, of the type **ge.graph.Graph**. |

## Constraints

Optionally declare the `context` parameter; when it is not declared, the original single-parameter calling method is retained.
