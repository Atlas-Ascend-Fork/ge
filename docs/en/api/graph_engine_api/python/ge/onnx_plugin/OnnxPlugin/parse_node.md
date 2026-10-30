# parse\_node

<!-- md-trans-meta sourceCommit=2591c34e5f149b4e71ec5a24ff34e4b4c36614bc translatedAt=2026-10-15T03:38:46.648Z pushedAt=2026-10-16T10:53:07.576Z -->

## Applicable Products

Supported on all chips.

## Description

Binds a one-to-one parameter parsing callback to the **descriptor**. During the parsing phase, after the parser matches the callback by **origin type**, it passes the ONNX node and the created **target operator** to the callback. The callback reads the ONNX node information and supplements the attributes and ports of the **target operator**.

## Prototype

```python
OnnxPlugin.parse_node(fn: Callable[..., None]) -> Callable[..., None]
```

## Parameters

| Parameter | Input/Output | Description |
| :--- | :--- | :--- |
| **fn** | Input | One-to-one parameter parsing callback to be bound. The type is a Python function, and the callback signature is `(node: OnnxNode, target: Operator) -> None`. `node` is a read-only value object of the ONNX node, and `target` is the target operator created by the parser. The callback supplements the attributes and ports of the target operator through the `set_attr` and `register_*` series of methods. |

## Return Value

Returns the original callback function. The decoration process does not change how the original function is called.

## Constraints

- The callback must return `None`. If any other value is returned, parsing of the current node fails.
- An exception raised inside the callback causes parsing of the current node to fail, and the target operator is not added to the graph.
- `target` is valid only during callback execution. Accessing it after the callback returns or raises an exception raises `RuntimeError`.
- When reading attributes through `node.attrs`, only scalars and lists of the same type are supported (`int`, `float`, `str`, and their corresponding lists). Composite-type attributes such as `TENSOR` and `GRAPH` are not supported. If a node contains such attributes, reading `attrs` as a whole fails. For type mapping and complete constraints, see [`attrs`](../OnnxNode/attrs.md). For such nodes, use [`parse_operator`](parse_operator.md) instead.
- If the target operator has a dynamic input/output prototype (such as `PartitionedCall`), you must call methods such as `register_input` and `register_output` in the callback to register ports; otherwise, the parser fails during the connection phase. If the target operator has a static prototype (such as `Elu`), port registration is not required.
- If the same descriptor is also bound with [`parse_operator`](parse_operator.md), the parser invokes only the callback bound by `parse_operator`, and this callback is ignored without raising an error. For the meaning of "bound simultaneously", see [Overview](overview.md).
- If `fn` is not a Python function (for example, a class instance, a built-in function, or another callable object), `TypeError` is raised.
- If the same descriptor is bound with a `parse_node` callback repeatedly, `ValueError` is raised.

## Example

The following example reads the `alpha` attribute of an ONNX node and writes it to the target operator:

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
    if not isinstance(alpha, (int, float)):
        raise TypeError("Elu alpha must be a number")
    target.set_attr("alpha", alpha)
```

The following example registers ports for a dynamic-input target operator based on the number of inputs of the ONNX node:

```python
from ge.graph import Operator
from ge.onnx_plugin import OnnxNode, onnx_plugin


sum_plugin = onnx_plugin(
    source="Sum",
    domain="ai.onnx",
    opsets=range(1, 23),
    target="Sum",
)


@sum_plugin.parse_node
def parse_sum(node: OnnxNode, target: Operator) -> None:
    count = len(node.inputs)
    if count == 0:
        raise ValueError("Sum requires at least one input")
    target.register_dynamic_input("x", count)
    target.set_attr("N", count)
```
