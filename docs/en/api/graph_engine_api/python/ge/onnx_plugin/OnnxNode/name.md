# name

<!-- md-trans-meta sourceCommit=500eefa22ce54caa25b8e15e68d19940c2524ed6 translatedAt=2026-10-15T03:34:51.976Z pushedAt=2026-10-16T10:53:07.563Z -->

## Applicable Products

Supported on all chips.

## Description

Gets the name of the ONNX node.

## Prototype

```python
onnx_node.name -> str
```

## Parameters

None

## Return Value

Returns the name of the ONNX node, of type `str`.

## Constraints

- **OnnxNode** is constructed by **GE** and passed in as a callback argument; users cannot construct it themselves.
- The attribute is read-only and supports only reading, not modification.

## Examples

```python
node_name = node.name
```
