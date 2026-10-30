# get\_format

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T03:25:49.097Z pushedAt=2026-10-16T10:53:07.523Z -->

## Applicable Products

Supported on all chips.

## Description

Obtains the **Format** of the Tensor described by **TensorDesc**.

## Prototype

```python
get_format() -> Format
```

## Parameters

None

## Return Value

Returns the **Format** enum value.

## Constraints

Because the returned **Format** information is a value copy, modifying the returned **Format** information does not affect the existing **Format** information in **TensorDesc**.
