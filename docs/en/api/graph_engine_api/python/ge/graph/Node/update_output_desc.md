# update\_output\_desc

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T03:13:47.663Z pushedAt=2026-10-16T10:53:07.472Z -->

## Applicable Products

Supported on all chips.

## Description

Updates the Tensor format of a specified output port.

## Prototype

```python
update_output_desc(index: int, tensor_desc: TensorDesc) -> None
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| index | Input | Output port index of the operator. |
| tensor_desc | Input | Tensor format to be updated. |

## Return Value

None

## Constraints

- If **index** is not an integer, a TypeError is raised.
- If **tensor\_desc** is not of the **TensorDesc** type, a TypeError is raised.
- If the Tensor format update fails, a RuntimeError is raised.
