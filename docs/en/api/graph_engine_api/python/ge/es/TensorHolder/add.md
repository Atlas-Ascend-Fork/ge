# add

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T02:54:07.614Z pushedAt=2026-10-16T10:53:07.392Z -->

## Applicable Products

Supported on all chips.

## Description

Adds two tensors.

## Prototype

```python
add(other: Union[TensorHolder, TensorLike]) -> TensorHolder
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| **other** | Input | Another tensor holder or a tensor-like object. |

## Return Value

\(TensorHolder\) Returns a new tensor holder representing the result.

## Constraints

- If **other** is neither a **TensorHolder** nor a **TensorLike**, a **TypeError** is raised.
- If the operation fails or the library is unavailable, a **RuntimeError** is raised.
- The + operator overloading is supported.
