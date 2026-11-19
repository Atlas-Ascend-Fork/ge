# attrs

<!-- md-trans-meta sourceCommit=500eefa22ce54caa25b8e15e68d19940c2524ed6 translatedAt=2026-10-15T03:34:47.971Z pushedAt=2026-10-16T10:53:07.560Z -->

## Applicable Products

Supported on all chips.

## Description

Obtains a read-only view of all attributes of an **ONNX** node.

## Prototype

```python
onnx_node.attrs -> Mapping[str, object]
```

## Parameters

None

## Return Value

Returns a read-only view of the dictionary keyed by attribute name, with attribute values converted to Python objects according to the ONNX attribute type:

| ONNX Attribute Type | Python Type |
| :--- | :--- |
| FLOAT | `float` |
| INT | `int` |
| STRING | `str` |
| FLOATS | `list[float]` |
| INTS | `list[int]` |
| STRINGS | `list[str]` |

## Constraints

- The returned view is read-only; modifying it raises `TypeError`.
- Attribute names must be non-empty. If an empty attribute name exists, parsing of the current node fails.
- Only the scalar and same-type list attributes listed in the table above are supported. Composite-type attributes such as **TENSOR**, **GRAPH**, and **SPARSE_TENSOR** are not supported, and parsing of the current node fails when such attributes are encountered.
- `attrs` is a whole view and has no interface for reading by a single attribute name. When a node contains the unsupported attributes described above, accessing `attrs` as a whole fails, and other ordinary attributes on the node cannot be read either. For such nodes, use the [`parse_operator`](../OnnxPlugin/parse_operator.md) callback instead to read the JSON-format attributes through the source operator.
- **OnnxNode** is constructed by **GE** and passed in as a callback argument; users cannot construct it themselves.

## Examples

```python
alpha = node.attrs.get("alpha", 1.0)
```
