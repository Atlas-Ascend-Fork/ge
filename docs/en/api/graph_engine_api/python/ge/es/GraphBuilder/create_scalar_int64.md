# create\_scalar\_int64

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T02:46:52.995Z pushedAt=2026-10-16T10:53:07.367Z -->

## Applicable Products

Supported on all chips.

## Description

Creates an **int64** scalar tensor.

## Prototype

```python
create_scalar_int64(value: int) -> TensorHolder
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| **value** | Input | Integer value. |

## Return Value

\(TensorHolder\) Returns a tensor holder that represents a scalar.

## Constraints

- If **value** is not an integer, a **TypeError** is thrown.
- If scalar creation fails, a **RuntimeError** is thrown.
