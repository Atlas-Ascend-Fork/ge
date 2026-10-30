# set\_output

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T04:08:05.192Z pushedAt=2026-10-16T10:53:07.681Z -->

## Applicable Products

Supported on all chips.

## Description

Sets the output anchor.

## Prototype

```python
set_output(node: Node, out_index: int) -> int
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| node | Input | Main graph node, of the type **ge.graph.Node**. |
| out_index | Input | Node output index. |

## Return Value

Returns the internal status (uint32 semantics).

## Constraints

None
