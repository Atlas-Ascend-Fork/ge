# parse\_operator

<!-- md-trans-meta sourceCommit=2eade100c61c3db3a52f948d9bb4884dac0147f0 translatedAt=2026-10-15T03:39:19.044Z pushedAt=2026-10-16T10:53:07.577Z -->

## Product Support

Supported on all chips.

## Description

Binds a parameter parsing callback based on **Operator** to the **descriptor**. During the parsing phase, the parser automatically creates a **source operator** and a **target operator** based on the ONNX node and passes both to the callback. The callback reads attributes and other information from the **source operator** and supplements the **target operator**.

## Prototype

```python
OnnxPlugin.parse_operator(fn: Callable[..., None]) -> Callable[..., None]
```

## Parameters

| Parameter | Input/Output | Description |
| :--- | :--- | :--- |
| fn | Input | The parameter parsing callback to be bound. The type is a Python function, and the callback signature is `(source: Operator, target: Operator) -> None`. `source` is the read-only operator automatically created by the parser based on the ONNX node, and `target` is the target operator. |

## Return Value

Returns the original callback function. The decoration process does not change how the original function is called.

## Constraints

- `source` is a read-only object. Calling `set_attr` or a port registration method on it raises `RuntimeError`; `get_attr`, `name`, and `type` can be used normally.
- Attribute reading convention for the source operator: the parser serializes the entire attribute list of the ONNX node into a JSON string and stores it in an attribute named `attribute`. The callback needs to parse it with `json.loads(source.get_attr("attribute"))` and then look up the attribute value by the `name` field.
- When the same descriptor is also bound with [`parse_node`](parse_node.md), the parser invokes only this callback; the callback bound by `parse_node` is ignored without raising an error (for the meaning of "bound simultaneously", see [Overview](overview.md)).
- The constraints on the callback return value, lifecycle, and exceptions are the same as those for [`parse_node`](parse_node.md).
- If `fn` is not a Python function (for example, an instance of a class, a built-in function, or another callable object), `TypeError` is raised.
- If the same descriptor is bound with the `parse_operator` callback repeatedly, `ValueError` is raised.

## Example

The following example copies the JSON attributes on the source operator to the target operator as a whole, completing attribute transfer without knowing the attribute names in advance:

```python
from ge.graph import Operator
from ge.onnx_plugin import onnx_plugin


plugin = onnx_plugin(
    source="GroupNormRelu",
    domain="example.domain",
    opsets=(1,),
    target="GroupNormRelu",
)


@plugin.parse_operator
def parse_group_norm(source: Operator, target: Operator) -> None:
    target.set_attr("attribute", source.get_attr("attribute"))
```

The JSON structure of the `attribute` attribute is shown below. In each entry, `name` is the attribute name, and `type` is the enumeration value of the attribute type (the value of the ONNX `AttributeProto`, for example, `1` indicates FLOAT). The value field is output according to the actual content of the attribute:

```json
{
  "attribute": [
    {"name": "alpha", "type": 1, "f": "1"}
  ]
}
```

The form of each value field in JSON corresponds to the attribute type. Use the field that actually exists in the entry:

- `f` (FLOAT type): string (the floating-point number is formatted with `%g`, with about 6 significant digits). Convert it to a numeric value when using it, for example, `float(item["f"])`.
- `i` (INT type): number.
- `s` (STRING type): string (the content is escaped; ordinary text can be used directly).
- `t` (TENSOR type) and `g` (GRAPH type): nested objects whose structure corresponds to the fields of the ONNX `TensorProto`/`GraphProto`. A tensor attribute means that the attribute value itself is a tensor (such as the `value` attribute of the Constant operator), and a subgraph attribute means that the attribute value itself is a subgraph (such as the `then_branch`/`else_branch` attributes of the control-flow operator If and the `body` attribute of Loop).
- `floats` and `ints` (list types): numeric arrays; `strings`: string array; `tensors` and `graphs`: object arrays. Note that the elements of `floats` are native numeric values, which differ from the string form of the scalar `f`; the elements of `strings` are not escaped, which differs from the scalar `s`.

Composite-type attributes (`t`, `g`, and their lists) are provided as nested objects and can be read and inspected. To convert them into GE objects such as `Tensor` for writing to the target operator, you need to decode them according to the field structure described above and then construct them; `set_attr` does not support directly passing a `dict`.

When a node has no attributes, the attribute named `attribute` does not exist on the source operator, and `get_attr("attribute")` raises a `RuntimeError`.

To read a single attribute by name, parse it and then look it up by the `name` field. The following is a snippet inside the callback that reads the `alpha` attribute of type FLOAT:

```python
import json

attrs = json.loads(source.get_attr("attribute"))["attribute"]
alpha = None
for item in attrs:
    if item["name"] == "alpha":
        alpha = float(item["f"])
        break
```
