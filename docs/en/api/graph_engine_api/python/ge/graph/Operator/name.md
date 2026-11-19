# name

<!-- md-trans-meta sourceCommit=500eefa22ce54caa25b8e15e68d19940c2524ed6 translatedAt=2026-10-15T03:14:29.774Z pushedAt=2026-10-16T10:53:07.473Z -->

## Applicable Products

Supported on all chips.

## Description

Obtains the operator name. In an ONNX Plugin callback, this name is the name of the ONNX node.

## Prototype

```python
operator.name -> str
```

## Parameters

None

## Return Value

Returns the operator name, of type `str`.

## Constraints

- Available only during callback execution. Calling it after the callback ends throws `RuntimeError`.
- The **Operator** object is constructed by **GE** and passed in as a callback argument. Users cannot construct it themselves.

## Example

```python
op_name = target.name
```
