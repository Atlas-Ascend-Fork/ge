# create\_scalar\_uint64

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T02:47:43.675Z pushedAt=2026-10-16T10:53:07.370Z -->

## Applicable Products

Supported on all chips.

## Description

Creates a uint64 scalar tensor.

## Prototype

```python
create_scalar_uint64(value: int) -> TensorHolder
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| **value** | Input | Integer value (must be non-negative). |

## Return Value

\(TensorHolder\) Represents the tensor holder of the scalar.

## Constraints

- If **value** is not an integer, **TypeError** is raised.
- If **value** is negative, **ValueError** is raised.
- If scalar creation fails, **RuntimeError** is raised.
