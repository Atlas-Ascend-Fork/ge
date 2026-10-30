# set\_shape

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T03:28:57.798Z pushedAt=2026-10-16T10:53:07.535Z -->

## Applicable Products

Supported on all chips.

## Description

Sets the tensor shape of **TensorDesc**.

## Prototype

```python
set_shape(shape: List[int]) -> TensorDesc
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| **shape** | Input | New tensor shape, of type `list[int]`. A **Shape** object can also be passed in. |

## Return Value

\(TensorDesc\) Returns self to support chained calls.

## Constraints

If **shape** is not a list of integers, it raises TypeError.
