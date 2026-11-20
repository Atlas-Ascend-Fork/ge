# create\_scalar\_int32

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T02:46:16.596Z pushedAt=2026-10-16T10:53:07.366Z -->

## Applicable Products

Supported on all chips.

## Description

Creates an int32 scalar tensor.

## Prototype

```python
create_scalar_int32(value: int) -> TensorHolder
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| **value** | Input | Integer value. |

## Return Value

\(TensorHolder\) Returns a tensor holder that represents a scalar.

## Constraints

- If **value** is not an integer, a **TypeError** is raised.
- If scalar creation fails, a **RuntimeError** is raised.
