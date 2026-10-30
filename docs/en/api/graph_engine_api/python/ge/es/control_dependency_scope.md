# control\_dependency\_scope

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T02:41:04.425Z pushedAt=2026-10-16T10:53:07.346Z -->

## Applicable Products

Supported on all chips.

## Description

Control dependency scope context manager.

## Prototype

```python
control_dependency_scope(tensors: List[TensorHolder])
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| **tensors** | Input | List of tensor holders. |

## Return Value

None

## Constraints

None

## Examples

```python
with control_dependency_scope([tensor1, tensor2]):
    tensor3 = builder.create_input(0)
```
