# get\_data

<!-- md-trans-meta sourceCommit=5f639ce5c040cc9acb4c51426dd643c1cfc70c83 translatedAt=2026-11-17T02:39:19.008Z pushedAt=2026-11-18T02:41:08.855Z nl=2 -->

## Applicable Products

Full chip support.

## Description

Obtains the tensor data.

## Prototype

```python
get_data() -> TensorLike
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
