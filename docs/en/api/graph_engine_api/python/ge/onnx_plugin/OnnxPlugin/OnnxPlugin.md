# OnnxPlugin

<!-- md-trans-meta sourceCommit=2591c34e5f149b4e71ec5a24ff34e4b4c36614bc translatedAt=2026-10-15T03:37:37.634Z pushedAt=2026-10-16T10:53:07.570Z -->

## Applicable Products

All chips supported.

## Description

The **OnnxPlugin** class is a descriptor of an ONNX Plugin, created by [`onnx_plugin()`](../onnx_plugin.md). It declares the mapping from ONNX original operators to **GE** target operators and binds parsing callbacks through the [`parse_node`](parse_node.md), [`parse_operator`](parse_operator.md), and [`decompose`](decompose.md) methods.

For a detailed description of the class, callback selection suggestions, and usage examples, see [Overview](overview.md).
