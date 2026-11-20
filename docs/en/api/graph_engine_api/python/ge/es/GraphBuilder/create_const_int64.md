# create\_const\_int64

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T02:43:39.305Z pushedAt=2026-10-16T10:53:07.358Z -->

## Applicable Products

Supported on all chips.

## Description

Creates an int64 constant tensor.

## Prototype

```python
create_const_int64(value: Union[int, List[int]], shape: Optional[List[int]] = None) -> TensorHolder
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| **value** | Input | A single integer or a list of integers. |
| **shape** | Input | Optional parameter. Shape dimensions. |

## Return Value

\(TensorHolder\) Returns the **TensorHolder** representing the constant.

## Constraints

- If it is a list and **shape** is provided, the number of elements must match the product of the **shape** dimensions.
- If **value** is not an int or a list of ints, **TypeError** is thrown.
- If the number of values does not match the shape, **ValueError** is thrown.
- If constant creation fails, **RuntimeError** is thrown.
