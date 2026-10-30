# onnx\_plugin

<!-- md-trans-meta sourceCommit=e57dde197e8bfdda0364328543edae0112fd3ca4 translatedAt=2026-10-15T03:36:59.327Z pushedAt=2026-10-16T10:53:07.571Z -->

## Applicable Products

Supported on all chips.

## Description

Creates a descriptor object for the ONNX Plugin to declare the mapping from ONNX original operators to GE target operators, and binds parsing callbacks through the descriptor's [`parse_node`](OnnxPlugin/parse_node.md), [`parse_operator`](OnnxPlugin/parse_operator.md), and [`decompose`](OnnxPlugin/decompose.md) methods.

## Prototype

```python
onnx_plugin(*, source: str, domain: str, opsets: Collection[int], target: str) -> OnnxPlugin
```

## Parameters

| Parameter | Input/Output | Description |
| :--- | :--- | :--- |
| source | Input | ONNX original operator type, for example, `Elu`. The type is a non-empty string and must not contain `:` (`:` is the field separator of the origin type, which is concatenated in the format `<domain>::<opset>::<source>`). |
| domain | Input | ONNX domain. Standard ONNX operators use `ai.onnx`. It must be explicitly declared during plugin registration. The type is a non-empty string and must not contain `:` (for the same reason as `source`). |
| opsets | Input | ONNX opset versions supported by the plugin. Types such as `list`, `tuple`, `set`, and `range` are supported, while `str` and `bytes` are not. Each element represents an opset version: `(1, 15)` means versions 1 and 15, and `range(1, 15)` means versions 1 through 14 (left-closed, right-open). Elements must be positive integers and are automatically deduplicated and sorted in ascending order during registration. |
| target | Input | GE target operator type. The type is non-empty string, and the corresponding operator prototype must already be installed and registered. |

## Return Value

Returns the `OnnxPlugin` descriptor object.

## Constraints

- Parameter validation is performed immediately when the function is called.
- If `source`, `domain`, or `target` is not a non-empty string, a `TypeError` is raised.
- If `source` or `domain` contains `:`, a `TypeError` is raised (`:` is the field separator of the origin type; its presence makes the origin type unable to be split by `<domain>::<opset>::<source>`).
- If the type of `opsets` is unsupported (such as `int` or a generator), or if it is `str` or `bytes`, a `TypeError` is raised.
- If `opsets` contains non-integer elements (including `bool`), a `TypeError` is raised.
- If `opsets` is empty or contains non-positive integer elements, a `ValueError` is raised.
- The descriptor expands `source`, `domain`, and `opsets` into a complete set of origin types (`domain::opset::source`, for example `ai.onnx::11::Elu`). The same origin type can be provided by only one C++ or Python plugin.
- The `domain` here is the declaration made by the plugin author at registration time and is unrelated to the domain field of the node in the ONNX model file: the domain of a node in the model file is allowed to be empty (the ONNX standard specifies that an empty value means the standard domain `ai.onnx`). For the display rules on the node side, see [origin_type](OnnxNode/origin_type.md).
- Origin type conflicts are reported during parser initialization: when a Python plugin conflicts with a C++ plugin, the C++ plugin is retained and the Python plugin is rejected; when multiple Python plugins conflict with each other, parser initialization fails.

## Calling Example

```python
from ge.onnx_plugin import onnx_plugin

elu = onnx_plugin(
    source="Elu",
    domain="ai.onnx",
    opsets=range(8, 19),
    target="Elu",
)
```
