# create\_scalar\_float

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T02:46:00.456Z pushedAt=2026-10-16T10:53:07.365Z -->

## Applicable Products

Supported on all chips.

## Description

Creates a float scalar tensor.

## Prototype

```python
create_scalar_float(value: float) -> TensorHolder
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| **value** | Input | Floating-point value. |

## Return Value

\(TensorHolder\) Returns the tensor holder that represents the scalar.

## Constraints

- If **value** is not a number, **TypeError** is thrown.
- If scalar creation fails, **RuntimeError** is thrown.
