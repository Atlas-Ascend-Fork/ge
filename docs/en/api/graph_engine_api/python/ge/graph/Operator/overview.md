# Overview

<!-- md-trans-meta sourceCommit=500eefa22ce54caa25b8e15e68d19940c2524ed6 translatedAt=2026-10-15T03:15:29.137Z pushedAt=2026-10-16T10:53:07.477Z -->

The Operator class is a controlled wrapper for GE operators during callbacks. It is passed by GE as an argument in ONNX Plugin callbacks ([`parse_node`](../../onnx_plugin/OnnxPlugin/parse_node.md), [`parse_operator`](../../onnx_plugin/OnnxPlugin/parse_operator.md), [`decompose`](../../onnx_plugin/OnnxPlugin/decompose.md)) and is used to read and write definition information such as operator attributes and inputs/outputs.

Object constraints:

- The Operator object is constructed by GE and passed into the callback. Users cannot construct it directly.
- The object is valid only during callback execution. It becomes invalid after the callback returns or raises an exception, and calling its methods afterward raises `RuntimeError`.
- The source operator in the `parse_operator` and `decompose` callbacks is a read-only object. `set_attr` and port registration methods raise `RuntimeError`.
- The object does not support copy and pickle.
- `ge.graph.Operator` and `ge.graph.Node` are different objects: `Operator` wraps the operator during parsing callbacks, while `Node` wraps a node that has been added to the graph. They do not share the underlying object.

Members:

- [`name`](name.md): Operator name.
- [`type`](type.md): Operator type.
- [`get_attr`](get_attr.md): Reads operator attributes.
- [`set_attr`](set_attr.md): Sets operator attributes.
- [`register_input`](register_input.md): Registers a required input port.
- [`register_optional_input`](register_optional_input.md): Registers an optional input port.
- [`register_output`](register_output.md): Registers an output port.
- [`register_dynamic_input`](register_dynamic_input.md): Registers a dynamic input port group.
- [`register_dynamic_output`](register_dynamic_output.md): Registers a dynamic output port group.

Example (in the `parse_node` callback, add attributes and ports for the target operator with dynamic input/output; `target` uses the dynamic input/output prototype `PartitionedCall`, whose ports must be registered in the callback):

```python
from ge.graph import Operator
from ge.onnx_plugin import OnnxNode, onnx_plugin


thresholded_relu = onnx_plugin(
    source="ThresholdedRelu",
    domain="example.domain",
    opsets=(1,),
    target="PartitionedCall",
)


@thresholded_relu.parse_node
def parse_thresholded_relu(node: OnnxNode, target: Operator) -> None:
    target.set_attr("alpha", node.attrs.get("alpha", 1.0))
    target.register_input("x")
    target.register_output("y")
```
