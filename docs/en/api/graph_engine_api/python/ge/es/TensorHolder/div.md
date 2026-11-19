# div

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T02:54:34.985Z pushedAt=2026-10-16T10:53:07.394Z -->

## Applicable Products

Supported on all chips.

## Description

Divides two tensors.

## Prototype

```python
div(other: Union[TensorHolder, TensorLike]) -> TensorHolder
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| **other** | Input | Another **TensorHolder** or **TensorLike** object. |

## Return Value

\(TensorHolder\) Returns a new **TensorHolder** representing the result.

## Constraints

- If **other** is not a **TensorHolder** or **TensorLike**, a **TypeError** is raised.
- If the operation fails or the library is unavailable, a **RuntimeError** is raised.
- The **/** operator overloading is supported.
