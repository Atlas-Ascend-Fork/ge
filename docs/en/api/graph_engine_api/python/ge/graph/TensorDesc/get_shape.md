# get\_shape

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T03:26:36.627Z pushedAt=2026-10-16T10:53:07.525Z -->

## Applicable Products

Supported on all chips.

## Description

Obtains the **Shape** of the tensor described by **TensorDesc**.

## Prototype

```python
get_shape() -> Shape
```

## Parameters

None

## Return Value

Returns a **Shape** object.

## Constraints

Because the returned **Shape** information is a value copy, modifying the returned **Shape** information does not affect the existing **Shape** information in **TensorDesc**.
