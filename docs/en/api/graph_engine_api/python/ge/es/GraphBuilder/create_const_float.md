# create\_const\_float

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T02:42:55.463Z pushedAt=2026-10-16T10:53:07.355Z -->

## Applicable Products

Supported on all chips.

## Description

Creates a float constant tensor.

## Prototype

```python
create_const_float(value: Union[float, List[float]], shape: Optional[List[int]] = None) -> TensorHolder
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| **value** | Input | A single floating-point number or a list of floating-point numbers. |
| **shape** | Input | Optional parameter, shape dimensions. |

## Return Value

\(TensorHolder\) Returns the **TensorHolder** representing the constant.

## Constraints

- If the value is a list and **shape** is provided, the number of elements must match the product of the **shape** dimensions.
- If **value** is not of type **float**, a list of **float**, or **int**, a **TypeError** is raised.
- If the number of values does not match the shape, a **ValueError** is raised.
- If constant creation fails, a **RuntimeError** is raised.
