# outputs

<!-- md-trans-meta sourceCommit=500eefa22ce54caa25b8e15e68d19940c2524ed6 translatedAt=2026-10-15T03:36:06.383Z pushedAt=2026-10-16T10:53:07.567Z -->

## Applicable Products

Supported on all chips.

## Description

Obtains the names of all output tensors of the ONNX node.

## Prototype

```python
onnx_node.outputs -> tuple
```

## Parameters

None

## Return Value

Returns a tuple of output tensor names, of type `tuple[str, ...]`, with elements in the same order as defined in the ONNX node.

## Constraints

- The returned tuple is a read-only snapshot. Modifying the tuple does not affect the ONNX node.
- **OnnxNode** is constructed by **GE** and passed in as a callback argument. Users cannot construct it themselves.

## Examples

```python
output_names = node.outputs
```
