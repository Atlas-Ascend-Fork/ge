# origin\_type

<!-- md-trans-meta sourceCommit=2591c34e5f149b4e71ec5a24ff34e4b4c36614bc translatedAt=2026-10-15T03:35:58.847Z pushedAt=2026-10-16T10:53:07.564Z -->

## Applicable Products

Supported on all chips.

## Description

Obtains the complete **origin type** of an ONNX node.

## Prototype

```python
onnx_node.origin_type -> str
```

## Parameters

None

## Return Value

Returns the complete origin type of the node, of type `str`, in the format `domain::opset::operator type`, for example `ai.onnx::11::Elu`. Here, `domain` is taken from the domain field of the node in the ONNX model file. This field may be empty; the ONNX standard specifies that an empty value means the standard domain `ai.onnx`, so it is displayed as `ai.onnx`. `opset` is the opset version of that domain in the model. Note: The plugin registration parameter [`domain`](../onnx_plugin.md) requires an explicit non-empty string, which is a different matter from the domain field of this node.

## Constraints

- **OnnxNode** is constructed by **GE** and passed in as a callback argument. Users cannot construct it themselves.
- The attribute is read-only. It supports only reading and does not support modification.

## Examples

```python
origin_type = node.origin_type
```
