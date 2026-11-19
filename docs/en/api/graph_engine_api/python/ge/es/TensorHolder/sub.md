# sub

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T02:57:21.217Z pushedAt=2026-10-16T10:53:07.407Z -->

## Applicable Products

Supported on all chips.

## Description

Subtracts two tensors.

## Prototype

```python
sub(other: Union[TensorHolder, TensorLike]) -> TensorHolder
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| **other** | Input | Another tensor holder or tensor-like object. |

## Return Value

Returns a new tensor holder that represents the result.

## Constraints

- If **other** is neither **TensorHolder** nor **TensorLike**, a **TypeError** is thrown.
- If the operation fails or the library is unavailable, a **RuntimeError** is thrown.
- The `-` operator overload is supported.
