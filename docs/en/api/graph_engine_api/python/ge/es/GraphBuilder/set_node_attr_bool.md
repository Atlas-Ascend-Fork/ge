# set\_node\_attr\_bool

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T02:51:55.569Z pushedAt=2026-10-16T10:53:07.384Z -->

## Applicable Products

Supported on all chips.

## Description

Sets a **bool** attribute for the node.

## Prototype

```python
set_node_attr_bool(tensor: TensorHolder, attr_name: str, value: bool) -> None
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| tensor | Input | Tensor holder object. |
| attr_name | Input | Attribute name. |
| value | Input | Boolean value. |

## Return Value

None

## Constraints

- If the parameter type is incorrect, a **TypeError** is thrown.
- If the setting fails, a **RuntimeError** is thrown.
