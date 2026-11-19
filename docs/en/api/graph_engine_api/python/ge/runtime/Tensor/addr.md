# addr

<!-- md-trans-meta sourceCommit=1e1ed868460cbcf8fd60b8a11149c952406370ba translatedAt=2026-10-15T04:10:04.828Z pushedAt=2026-10-16T10:53:07.690Z -->

## Applicable Products

Supported on all chips.

## Description

Obtains the runtime address of the **Tensor** data.

## Prototype

```python
tensor.addr -> int
```

## Parameters

None

## Return Value

Returns the integer representation of the **Tensor** data address.

## Constraints

- `Tensor` is provided by the runtime and can only be used during the current execution callback.
- This address is used only for constructing runtime execution parameters. It must not be read, written, or freed in Python.

## Example

```python
def execute(self, x: Tensor, y: Tensor) -> None:
    address = x.addr
```
