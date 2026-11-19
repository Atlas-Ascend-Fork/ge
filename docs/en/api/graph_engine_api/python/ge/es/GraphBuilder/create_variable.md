# create\_variable

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T02:47:53.119Z pushedAt=2026-10-16T10:53:07.371Z -->

## Applicable Products

Supported on all chips.

## Description

Creates a variable tensor.

## Prototype

```python
create_variable(index: int, name: str) -> TensorHolder
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| index | Input | Variable index. |
| name | Input | Variable name. |

## Return Value

\(TensorHolder\) Represents the tensor holder of the variable.

## Constraints

- If the parameter type is incorrect, a **TypeError** is thrown.
- If variable creation fails, a **RuntimeError** is thrown.
