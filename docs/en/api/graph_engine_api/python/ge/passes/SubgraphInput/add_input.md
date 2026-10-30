# add\_input

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T04:07:08.911Z pushedAt=2026-10-16T10:53:07.675Z -->

## Applicable Products

Supported on all chips.

## Description

Adds an input anchor (node output index).

## Prototype

```python
add_input(node: Node, out_index: int) -> int
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| **node** | Input | Main graph node, of the type **ge.graph.Node**. |
| **out_index** | Input | Node output index. |

## Return Value

Returns the internal index.

## Constraints

None
