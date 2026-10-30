# Shape Constructor

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T03:20:01.896Z pushedAt=2026-10-16T10:53:07.500Z -->

## Applicable Products

Supported on all chips.

## Description

Creates a new **Shape** object.

## Prototype

```python
Shape(dims: Optional[List[int]] = None)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| dims | Input | Value content of **Shape**. **Shape** represents the dimension sizes of tensor data, and **List[int]** represents the specific size of each dimension. |

## Return Value

Returns a **Shape** object.

## Constraints

If **dims** is not a list of integers, a **TypeError** is thrown.
