# Description

<!-- md-trans-meta sourceCommit=2591c34e5f149b4e71ec5a24ff34e4b4c36614bc translatedAt=2026-10-15T03:38:44.259Z pushedAt=2026-10-16T10:53:07.574Z -->

The **OnnxPlugin** class is the descriptor of an ONNX plugin. A descriptor instance declares a mapping from an ONNX primitive operator to a GE target operator and holds the bound parsing callbacks. It is created by [`onnx_plugin()`](../onnx_plugin.md).

A descriptor supports binding the following callbacks:

- [`parse_node`](parse_node.md): A one-to-one parameter parsing callback, corresponding to [`ParseParamsFn`](../../../../../../user_guides/custom_op/custom_op_v2/custom_op_development_guide.md#64-onnx-graph-entry) in C++.
- [`parse_operator`](parse_operator.md): An Operator-based parameter parsing callback, corresponding to [`ParseParamsByOperatorFn`](../../../../../../user_guides/custom_op/custom_op_v2/custom_op_development_guide.md#64-onnx-graph-entry) in C++.
- [`decompose`](decompose.md): A one-to-many decomposition callback, corresponding to [`ParseOpToGraphFn`](../../../../../../user_guides/custom_op/custom_op_v2/custom_op_development_guide.md#64-onnx-graph-entry) in C++.

The three types of callbacks correspond to different stages of the parsing process and can be combined as needed:

- `parse_node` and `parse_operator` belong to the same parameter parsing stage, so bind either one of them. "Binding both" means calling [`parse_node`](parse_node.md) first and then [`parse_operator`](parse_operator.md) on the same descriptor to bind one callback each: during the parsing stage, only the callback bound by `parse_operator` is invoked, while the callback bound by `parse_node` is ignored without raising an error. Therefore, do not bind both at the same time.
- `decompose` is independent of the parameter parsing stage. It can be combined with either parameter parsing callback, or bound alone.

Callbacks are bound through decorators. A decorator returns the original function and completes descriptor registration when binding the callback. Binding the same type of callback repeatedly raises `ValueError`. After a descriptor is created, `source`, `domain`, `opsets`, and `target` cannot be modified.

Callback selection suggestions:

- When only parameter parsing is required, prefer `parse_node`: it reads ONNX node attributes directly by attribute name, which is the most intuitive.
- When node attributes contain composite types such as tensors or subgraphs, only `parse_operator` can be used: [`OnnxNode.attrs`](../OnnxNode/attrs.md) does not support composite-type attributes, and reading a node with such attributes fails entirely.
- When the target operator has no corresponding implementation in GE and must be expressed by combining existing operators, bind `decompose` and build a substitute subgraph in the callback.

Example:

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
    target.set_attr("alpha", node.attrs.get("alpha", 1.0))
```
