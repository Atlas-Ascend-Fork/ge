# get\_placement

<!-- md-trans-meta sourceCommit=5f639ce5c040cc9acb4c51426dd643c1cfc70c83 translatedAt=2026-11-17T02:39:24.131Z pushedAt=2026-11-18T02:41:08.856Z nl=2 -->

## Applicable Products

Supported on all chips.

## Description

Obtains the storage location of the data.

## Prototype

```python
get_placement() -> Placement
```

## Parameters

None

## Return Value

\(Placement\)The storage location of the Tensor data.

## Constraints

- If the return value is not a valid **Placement**, it raises **ValueError**.
