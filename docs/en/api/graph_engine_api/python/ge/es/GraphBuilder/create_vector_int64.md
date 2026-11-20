# create\_vector\_int64

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T02:48:30.773Z pushedAt=2026-10-16T10:53:07.372Z -->

## Applicable Products

Supported on all chips.

## Description

Creates an **int64** vector tensor.

## Prototype

```python
create_vector_int64(value: List[int]) -> TensorHolder
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| **value** | Input | List of integers. |

## Return Value

\(TensorHolder\) Represents the tensor holder of the vector.

## Constraints

- If **value** is not a list of integers, a **TypeError** is thrown.
- If vector creation fails, a **RuntimeError** is thrown.
