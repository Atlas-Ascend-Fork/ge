# set\_shape

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T02:57:16.302Z pushedAt=2026-10-16T10:53:07.404Z -->

## Applicable Products

Supported on all chips.

## Description

Sets the tensor shape.

## Prototype

```python
set_shape(shape: List[int]) -> TensorHolder
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| **shape** | Input | List of shape dimensions. |

## Return Value

\(TensorHolder\) Returns itself to support chained calls.

## Constraints

- If **shape** is not a list of integers, a **TypeError** is thrown.
- If not all shape dimensions are integers, a TypeError is thrown.
- If the operation fails, a **RuntimeError** is thrown.
