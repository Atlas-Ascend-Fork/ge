# create\_scalar\_uint32

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T02:47:09.162Z pushedAt=2026-10-16T10:53:07.368Z -->

## Applicable Products

Supported on all chips.

## Description

Creates a **uint32** scalar tensor.

## Prototype

```python
create_scalar_uint32(value: int) -> TensorHolder
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| value | Input | Integer value (must be within the uint32 range). |

## Return Value

\(TensorHolder\) Represents the tensor holder of the scalar.

## Constraints

- If **value** is not an integer, a **TypeError** is thrown.
- If **value** is not within the \[0, 2^32-1\] range, a **ValueError** is thrown.
- If scalar creation fails, a **RuntimeError** is thrown.
