# NodeIo

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T03:49:08.680Z pushedAt=2026-10-16T10:53:07.591Z -->

## Applicable Products

Supported on all chips.

## Description

Describes a node output anchor in the form of a frozen dataclass.

## Prototype

```python
@dataclass(frozen=True)
class NodeIo:
    node: Node
    index: int = 0
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| node | Input | Node object of type **ge.graph.Node**. |
| index | Input | Node output index, defaulting to 0. |

## Return Value

None

## Constraints

None
