# create\_const\_int32

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T02:43:27.004Z pushedAt=2026-10-16T10:53:07.357Z -->

## Applicable Products

Supported on all chips.

## Description

Creates an **int32** constant tensor.

## Prototype

```python
create_const_int32(value: Union[int, List[int]], shape: Optional[List[int]] = None) -> TensorHolder
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| value | Input | A single integer or a list of integers. |
| shape | Input | Optional parameter, shape dimensions. |

## Return Value

\(TensorHolder\) Returns a tensor holder that represents the constant.

## Constraints

- If a list is provided together with **shape**, the number of elements must match the product of the **shape** dimensions.
- If **value** is not an int or a list of ints, **TypeError** is raised.
- If the number of values does not match the shape, **ValueError** is raised.
- If constant creation fails, **RuntimeError** is raised.
