# \_\_init\_\_

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T04:06:50.474Z pushedAt=2026-10-16T10:53:07.674Z -->

## Applicable Products

Supported on all chips.

## Description

Constructs a **SubgraphInput**. Two construction methods are supported: constructing an empty input (and then appending anchors via **add\_input**), or constructing from an \(node, out\_index\) iterable.

## Prototype

```python
__init__() -> None
__init__(node_inputs: Iterable[tuple[Node, int]]) -> None
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| **node_inputs** | Input | Optional. An iterable of `(node, out_index)` tuples. Each element must be a 2-tuple; otherwise, a `RuntimeError` is thrown at the C++ layer. |

## Return Value

None

## Constraints

When **node\_inputs** is passed in, each element must be a 2-tuple; otherwise, the C++ layer throws **RuntimeError**.
