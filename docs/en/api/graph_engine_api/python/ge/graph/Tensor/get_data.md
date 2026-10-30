# get\_data

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T03:21:03.389Z pushedAt=2026-10-16T10:53:07.504Z -->

## Applicable Products

Full chip support.

## Description

Obtains the tensor data.

## Prototype

```python
get_data() -> TensorLike
```

## Parameters

None

## Return Value

Returns the [TensorLike type](#tensorlike-type) tensor data.

## Constraints

If the acquisition fails, a RuntimeError is raised.

## TensorLike Type

**TensorLike** is a type alias that represents a tensor-like object:

- A single number (int or float)
- A list of numbers (nested list)

It is defined as follows:

```python
TensorLike = Union[int, float, List['TensorLike']]
```
