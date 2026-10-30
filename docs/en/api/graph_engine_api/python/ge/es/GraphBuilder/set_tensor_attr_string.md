# set\_tensor\_attr\_string

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T02:53:42.769Z pushedAt=2026-10-16T10:53:07.391Z -->

## Applicable Products

Supported on all chips.

## Description

Sets a string attribute for a tensor.

## Prototype

```python
set_tensor_attr_string(tensor: TensorHolder, attr_name: str, value: str) -> None
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| tensor | Input | Tensor holder object. |
| attr_name | Input | Attribute name. |
| value | Input | String value. |

## Return Value

None

## Constraints

- If the parameter type is incorrect, a **TypeError** is thrown.
- If the setting fails, a **RuntimeError** is thrown.
