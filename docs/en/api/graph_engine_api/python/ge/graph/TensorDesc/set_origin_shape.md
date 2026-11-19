# set\_origin\_shape

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T03:28:37.007Z pushedAt=2026-10-16T10:53:07.534Z -->

## Applicable Products

Supported on all chips.

## Description

Sets the original shape of the Tensor in **TensorDesc**. This shape refers to the shape of the original network model.

## Prototype

```python
set_origin_shape(shape: List[int]) -> TensorDesc
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| **shape** | Input | Original tensor shape, of type **list[int]**. A **Shape** object can be passed in. |

## Return Value

\(TensorDesc\) Returns itself to support chained calls.

## Constraints

If shape is not a list of integers, **TypeError** is raised.
