# type

<!-- md-trans-meta sourceCommit=500eefa22ce54caa25b8e15e68d19940c2524ed6 translatedAt=2026-10-15T03:18:20.316Z pushedAt=2026-10-16T10:53:07.492Z -->

## Applicable Products

Supported on all chips.

## Description

Obtains the operator type. In an ONNX Plugin callback, this type is the GE target operator type specified by the `target` parameter in the `descriptor`.

## Prototype

```python
operator.type -> str
```

## Parameters

None

## Return Value

Returns the operator type, of type `str`.

## Constraints

- Available only during callback execution. Calling it after the callback ends throws `RuntimeError`.
- The **Operator** object is constructed by GE and passed in as a callback argument. Users cannot construct it themselves.

## Examples

```python
op_type = target.type
```
