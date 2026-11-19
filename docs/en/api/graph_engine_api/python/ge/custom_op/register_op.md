# register\_op

<!-- md-trans-meta sourceCommit=d1d4ddfcd8d47cc6e1d8cdd0eb7c50848ce5580d translatedAt=2026-11-17T02:40:34.256Z pushedAt=2026-11-18T02:41:08.867Z nl=2 -->

## Applicable Products

Supported on all chips.

## Description

Registers a custom operator prototype and describes the operator inputs, attributes, and outputs through the type annotations of the decorated function.

## Prototype

```python
register_op(*, op_type: str, mutates_args=(), output_names=None) -> callable
```

## Parameters

| Parameter | Input/Output | Description |
| :--- | :--- | :--- |
| op_type | Input | Custom operator type. The type is a non-empty string. The same `op_type` can correspond to only one prototype. When the exact same prototype is registered repeatedly, the existing registration is reused; when a different prototype is registered, `ValueError` is raised. |
| mutates_args | Input | Declaration of the output-to-reference-input relationship, an optional parameter. Supports a string list or tuple declared in output order, and also supports a list or tuple of two-element tuples consisting of an input name and an output index. The default value is `()`. The two forms cannot be mixed. |
| output_names | Input | Output names, an optional parameter of list or tuple type. Names are specified in the order of operator outputs, and the number of names must be consistent with the number of outputs declared by the return type annotation. The default value is `None`, which indicates automatic naming. When an output has the same name as an input, it indicates a reference to that input. |

`mutates_args` supports the following two forms:

- Sequential form: `("x", "y")` indicates that output index 0 (the first output) references input `x`, and output index 1 (the second output) references input `y`.
- Explicit form: `(("x", 1),)` indicates that output index 1 (the second output) references input `x`.

When outputs reference inputs in order, the sequential form is recommended. When certain outputs need to be skipped, the explicit form can be used to specify the output index.

## Return Value

Returns a function decorator. After the decorator completes prototype registration, it returns the original function without changing how the original function is called, and sets the `__ge_op_proto_descriptor__` attribute for the original function.

- A `TypeError` is raised when `op_type`, the decorated object, the function signature, the type annotation, the attribute default value, the `mutates_args` format, or the `output_names` type is invalid.
- A `TypeError` is raised when a name in `output_names` is empty or contains a null character; a `ValueError` is raised when the number of names does not match the number of outputs or when names are duplicated.
- A `ValueError` is raised when `mutates_args` contains duplicate, out-of-range, or nonexistent input names, or when a prototype registration conflict occurs.
- A `ValueError` is raised when `output_names` and `mutates_args` are set at the same time.

## Constraints

The decorated function is the Meta derivation function of a custom operator, and its signature declares the operator inputs (Input), attributes (Attr), and outputs (Output). During registration, the decorated function is validated as follows:

- The decorated object must be a Python function, and variable positional arguments and variable keyword arguments are not supported.
- Input: Positional parameters represent operator inputs. They must provide type annotations and cannot have default values. The supported types are `TensorDesc`, `Optional[TensorDesc]`, and `List[TensorDesc]`.
- Attr: Keyword-only parameters after `*` represent operator attributes. They must provide type annotations. The supported types are `int`, `float`, `bool`, `str`, `DataType`, `Tensor`, `List[int]`, `List[float]`, `List[bool]`, `List[str]`, `List[DataType]`, and `List[List[int]]`. Attribute default values must strictly match the type annotations, and `Tensor` attributes cannot have default values.
- Output: The return type annotation represents operator outputs. A return type annotation must be provided. The supported types are `None`, `TensorDesc`, `List[TensorDesc]`, and a `Tuple` composed of `TensorDesc` and `List[TensorDesc]`.
- The input names referenced by `mutates_args` must exist. Each input name and output index can be used at most once, output indices must not be out of range, and the sequential form and the explicit form cannot be mixed.
- Each name in `output_names` must be a non-empty string that does not contain a null character, and output names cannot be duplicated.
- Setting `mutates_args` and `output_names` at the same time is not supported.
- When the same function registers the same prototype repeatedly, the existing registration is reused. If the prototype content changes or another function uses the already-registered `op_type`, registration fails.

## Examples

The following example registers the `AddCustom` operator prototype. `x` and `y` are required inputs. The return type annotation indicates that the operator has one output, with the default name `output0`.

```python
from ge.custom_op import register_op
from ge.runtime import TensorDesc


@register_op(op_type="AddCustom")
def infer_add(x: TensorDesc, y: TensorDesc) -> TensorDesc:
    return x
```

The following example registers the `InplaceAddCustom` operator prototype. `x` and `y` are required inputs, and `alpha` is an optional attribute with a default value of `1.0`. The return type annotation indicates that the operator has one output. `mutates_args=("x",)` indicates that the first output references input `x`, and the output name is `x`.

```python
from ge.custom_op import register_op
from ge.runtime import TensorDesc


@register_op(op_type="InplaceAddCustom", mutates_args=("x",))
def infer_meta(
    x: TensorDesc,
    y: TensorDesc,
    *,
    alpha: float = 1.0,
) -> TensorDesc:
    return x
```

The following example registers the `NamedOutputsCustom` operator prototype. `x` is a required input. The return type annotation declares three outputs, and `output_names` names them `value`, `parts`, and `x` in order. Among them, `parts` is a group of dynamic outputs, and output `x` references the input with the same name.

```python
from typing import List, Tuple

from ge.custom_op import register_op
from ge.runtime import TensorDesc


@register_op(op_type="NamedOutputsCustom", output_names=("value", "parts", "x"))
def infer_named_outputs(
    x: TensorDesc,
) -> Tuple[TensorDesc, List[TensorDesc], TensorDesc]:
    return x, [x], x
```
