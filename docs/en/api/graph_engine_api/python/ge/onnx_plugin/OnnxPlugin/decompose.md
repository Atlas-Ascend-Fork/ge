# decompose

<!-- md-trans-meta sourceCommit=500eefa22ce54caa25b8e15e68d19940c2524ed6 translatedAt=2026-10-15T03:37:45.660Z pushedAt=2026-10-16T10:53:07.573Z -->

## Applicable Products

Supported on all chips.

## Description

Binds a one-to-many decomposition callback to **descriptor**. During the parsing phase, the parser passes the source operator to the callback. The callback uses the ES graph construction interface to build a replacement subgraph and returns it, and the parser expands the subgraph into the graph to replace the single target operator.

## Prototype

```python
OnnxPlugin.decompose(fn: Callable[..., object]) -> Callable[..., object]
```

## Parameters

| Parameter | Input/Output | Description |
| :--- | :--- | :--- |
| fn | Input | The bound one-to-many decomposition callback. The type is a Python function, and the callback signature is `(source: Operator) -> Graph`. `source` is a read-only view of the operator produced during parameter parsing, and the return value is the replacement subgraph of type `ge.graph.Graph`. |

## Return Value

Returns the original callback function. The decoration process does not change how the original function is called.

## Constraints

- The callback return value must be a `ge.graph.Graph` object. If any other value (including `None`) is returned, parsing of the current node fails.
- `source` is a read-only object. Calling `set_attr` or a port registration method on it raises `RuntimeError`.
- The attributes and ports of `source` come from the output of the parameter parsing phase (the [`parse_node`](parse_node.md) or [`parse_operator`](parse_operator.md) callback): the attributes read by `get_attr` and the registered ports are all written by that callback. When the target operator is a dynamic input/output prototype, port registration must be completed in the parameter parsing callback.
- In the decomposition scenario, the target operator serves only as a placeholder and is replaced by the substitute subgraph after parsing is complete. When used together with the parameter parsing callback, `target` is usually `PartitionedCall`, a general-purpose operator in GE that carries subgraphs, performs no computation itself, and has flexible port definitions, making it convenient to register ports in the parameter parsing callback as required by the decomposition.
- When only `decompose` is bound (without binding any parameter parsing callback), the parameter parsing phase writes only the parser's internal attributes and does not register ports: `source` has no readable business attributes, and the target operator cannot be a dynamic input/output prototype (such as `PartitionedCall`); otherwise, the parser fails during the connection phase. In this case, a `target` whose ports are statically defined in the operator prototype should be selected.
- Graph construction uses ES graph construction interfaces such as [`GraphBuilder`](../../es/GraphBuilder/GraphBuilder.md). `GraphBuilder` is created inside the callback. Graph construction operators such as `ge.es.math` and `ge.es.nn` come from the external ES plugin package, which must be installed before use.
- If `fn` is not a Python function (for example, a class instance, a built-in function, or another callable object), `TypeError` is raised.
- If the `decompose` callback is bound repeatedly to the same descriptor, `ValueError` is raised.

## Example

The following example decomposes the **ThresholdedRelu** node into two operators, `Threshold` and `Mul` (for the complete sample, see [ONNX Plugin Sample](../../../../../../../../examples/onnx_plugin/README.md)):

```python
from ge.es import GraphBuilder
from ge.es.math import Mul
from ge.es.nn import Threshold
from ge.graph import Graph, Operator
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


@thresholded_relu.decompose
def decompose_thresholded_relu(source: Operator) -> Graph:
    alpha = float(source.get_attr("alpha"))
    builder = GraphBuilder("thresholded_relu_decomposition")
    x = builder.create_input(0)
    mask = Threshold(x, threshold=alpha)
    output = Mul(x, mask)
    return builder.build_and_reset([output])
```
