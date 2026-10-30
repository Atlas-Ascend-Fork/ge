# update\_input\_desc

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T03:13:38.333Z pushedAt=2026-10-16T10:53:07.470Z -->

## Applicable Products

Supported on all chips.

## Description

Updates the Tensor format of the specified input port.

## Prototype

```python
update_input_desc(index: int, tensor_desc: TensorDesc) -> None
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| **index** | Input | Index of the input port to be updated. |
| **tensor_desc** | Input | Tensor format to be updated. |

## Return Value

None

## Constraints

- If **index** is not an integer, **TypeError** is thrown.
- If tensor\_desc is not of the **TensorDesc** type, **TypeError** is thrown.
- If the Tensor format update fails, **RuntimeError** is thrown.
