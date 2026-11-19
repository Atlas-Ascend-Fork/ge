# Overview

<!-- md-trans-meta sourceCommit=500eefa22ce54caa25b8e15e68d19940c2524ed6 translatedAt=2026-10-15T03:36:07.655Z pushedAt=2026-10-16T10:53:07.566Z -->

The **OnnxNode** class is a read-only value object of an ONNX node. It is constructed by GE during the parsing phase based on the `NodeProto` of the ONNX model, and passed as an argument to the [`parse_node`](../OnnxPlugin/parse_node.md) callback (the other argument is an [`Operator`](../../graph/Operator/overview.md) object). The object is created and destroyed by GE; users do not need to and cannot construct it themselves.

**OnnxNode** provides only the following read-only attributes and does not provide any modification methods:

- [`name`](name.md): ONNX node name.
- [`origin_type`](origin_type.md): Full origin type of the node.
- [`inputs`](inputs.md): Tuple of input tensor names.
- [`outputs`](outputs.md): Tuple of output tensor names.
- [`attrs`](attrs.md): Read-only view of node attributes.

Access example (inside the `parse_node` callback):

```python
from ge.graph import Operator
from ge.onnx_plugin import OnnxNode, onnx_plugin


elu = onnx_plugin(
    source="Elu",
    domain="ai.onnx",
    opsets=range(8, 19),
    target="Elu",
)


@elu.parse_node
def parse_elu(node: OnnxNode, target: Operator) -> None:
    alpha = node.attrs.get("alpha", 1.0)
    target.set_attr("alpha", alpha)
```
