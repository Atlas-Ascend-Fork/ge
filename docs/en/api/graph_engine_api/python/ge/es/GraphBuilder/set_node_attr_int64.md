# set\_node\_attr\_int64

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T02:52:11.821Z pushedAt=2026-10-16T10:53:07.386Z -->

## Applicable Products

Supported on all chips.

## Description

Sets the **int64** attribute for a node.

## Prototype

```python
set_node_attr_int64(tensor: TensorHolder, attr_name: str, value: int) -> None
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| **tensor** | Input | Tensor holder object. |
| **attr_name** | Input | Attribute name. |
| **value** | Input | Integer value. |

## Return Value

None

## Constraints

- If the parameter type is incorrect, a **TypeError** is thrown.
- If the setting fails, a **RuntimeError** is thrown.
