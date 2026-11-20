# size

<!-- md-trans-meta sourceCommit=1e1ed868460cbcf8fd60b8a11149c952406370ba translatedAt=2026-10-15T04:13:51.491Z pushedAt=2026-10-16T10:53:07.705Z -->

## Applicable Products

Supported on all chips.

## Description

Obtains the memory size occupied by **Tensor** data.

## Prototype

```python
tensor.size -> int
```

## Parameters

None

## Return Value

Returns the number of bytes occupied by the **Tensor** data.

## Constraints

The return value indicates the runtime storage space size, which is not necessarily equal to the number of elements in **OriginShape** multiplied by the number of bytes per element.

## Examples

```python
byte_size = x.size
```
