# get\_tensor\_desc

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T03:22:14.206Z pushedAt=2026-10-16T10:53:07.508Z -->

## Applicable Products

Supported on all chips.

## Description

Obtains the descriptor of **Tensor**.

## Prototype

```python
get_tensor_desc() -> TensorDesc
```

## Parameters

None

## Return Value

Returns the descriptor of the current **Tensor**.

## Constraints

- If the **Tensor** format fails to be obtained, a **RuntimeError** is thrown.
- Modifying the returned **TensorDesc** information does not affect the existing **TensorDesc** information in the **Tensor** object.
