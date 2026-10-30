# add\_control\_dependency

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T02:42:29.909Z pushedAt=2026-10-16T10:53:07.351Z -->

## Applicable Products

Supported on all chips.

## Description

Adds a control dependency from src\_tensors to dst\_tensor.

## Prototype

```python
add_control_dependency(dst_tensor: TensorHolder, src_tensors: List[TensorHolder]) -> None
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| **dst_tensor** | Input | Target tensor holder. |
| **src_tensors** | Input | List of source tensor holders. |

## Return Value

None

## Constraints

- If the parameter type is incorrect, a **TypeError** is thrown.
- If the addition fails, a **RuntimeError** is thrown.
