# get\_placement

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T03:21:32.751Z pushedAt=2026-10-16T10:53:07.506Z -->

## Applicable Products

Supported on all chips.

## Description

Obtains the storage location of the data.

## Prototype

```python
get_placement() -> Placement
```

## Parameters

None

## Return Value

\(Placement\)The storage location of the Tensor data.

## Constraints

- If the return value is not a valid **Placement**, it raises **ValueError**.
