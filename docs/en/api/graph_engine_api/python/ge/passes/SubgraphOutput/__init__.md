# \_\_init\_\_

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T04:07:29.365Z pushedAt=2026-10-16T10:53:07.677Z -->

## Applicable Products

Supported on all chips.

## Description

Constructs a **SubgraphOutput**. Two construction methods are supported: constructing an empty output (then calling set\_output to set it), or directly binding an output anchor.

## Prototype

```python
__init__() -> None
__init__(node: Node, out_index: int) -> None
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| node | Input | Optional. Main graph node, of the type **ge.graph.Node**. |
| out_index | Input | Optional. Node output index. |

## Return Value

None

## Constraints

None
