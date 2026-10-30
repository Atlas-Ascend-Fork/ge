# set\_tensor\_attr\_int64

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T02:53:27.540Z pushedAt=2026-10-16T10:53:07.390Z -->

## Applicable Products

Supported on all chips.

## Description

Sets an int64 attribute for a tensor.

## Prototype

```python
set_tensor_attr_int64(tensor: TensorHolder, attr_name: str, value: int) -> None
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| tensor | Input | Tensor holder object. |
| attr_name | Input | Attribute name. |
| value | Input | Integer value. |

## Return Value

None

## Constraints

- If the parameter type is incorrect, a TypeError is thrown.
- If the setting fails, a RuntimeError is thrown.
