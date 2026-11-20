# set\_attr

<!-- md-trans-meta sourceCommit=2591c34e5f149b4e71ec5a24ff34e4b4c36614bc translatedAt=2026-10-15T03:18:10.747Z pushedAt=2026-10-16T10:53:07.491Z -->

## Applicable Products

Supported on all chips.

## Description

Sets the value of the attribute with the specified name on the operator.

## Prototype

```python
Operator.set_attr(name: str, value: object) -> None
```

## Parameters

| Parameter | Input/Output | Description |
| :--- | :--- | :--- |
| name | Input | Attribute name, of the non-empty string type. |
| value | Input | Attribute value. Supports `int`, `float`, `bool`, `str`, `DataType`, `Tensor`, and a `list` with consistent element types (elements are numeric values, `bool`, `str`, or `DataType`). The list must be of the `list` type and non-empty, and the elements of a numeric list can be `int` or `float`. `DataType` is the GE data type enumeration (such as `DT_FLOAT` and `DT_INT32`) that describes the tensor element type. For enumeration values, see [DataType](../../DataType.md). |

## Return Value

None

## Constraints

- Available only during callback execution. Calling it after the callback ends throws `RuntimeError`.
- The source operator in the [`parse_operator`](../../onnx_plugin/OnnxPlugin/parse_operator.md) and [`decompose`](../../onnx_plugin/OnnxPlugin/decompose.md) callbacks is a read-only object. Calling it throws `RuntimeError`.
- Throws `TypeError` when `name` is not a non-empty string.
- Throws `ValueError` when `value` is of an unsupported type (including an empty list, a list with inconsistent element types, and so on).
- This API does not validate attribute names. No error is reported when an attribute name is not declared in the operator prototype, and the issue may only be discovered in the subsequent graph validation phase.

## Examples

```python
target.set_attr("alpha", 1.0)
```
