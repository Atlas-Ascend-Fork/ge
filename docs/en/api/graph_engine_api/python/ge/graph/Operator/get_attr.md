# get\_attr

<!-- md-trans-meta sourceCommit=2591c34e5f149b4e71ec5a24ff34e4b4c36614bc translatedAt=2026-10-15T03:14:23.148Z pushedAt=2026-10-16T10:53:07.475Z -->

## Applicable Products

Supported on all chips.

## Description

Reads the value of the attribute with the specified name on the operator.

## Prototype

```python
Operator.get_attr(name: str) -> object
```

## Parameters

| Parameter | Input/Output | Description |
| :--- | :--- | :--- |
| **name** | Input | Attribute name, of the non-empty string type. |

## Return Value

Returns the attribute value. The specific type depends on the storage type of the attribute, supporting `int`, `float`, `bool`, `str`, `DataType`, `Tensor`, and lists of the corresponding element types. `DataType` is the GE data type enumeration (such as `DT_FLOAT` and `DT_INT32`) that describes the tensor element type. For the enumeration values, see [DataType](../../DataType.md).

## Constraints

- Available only during callback execution. Calling it after the callback ends throws `RuntimeError`.
- The source operator in the [`parse_operator`](../../onnx_plugin/OnnxPlugin/parse_operator.md) and [`decompose`](../../onnx_plugin/OnnxPlugin/decompose.md) callbacks is a read-only object. You can only read attributes from it, not write to it.
- Throws `TypeError` when `name` is not a non-empty string.
- Throws `RuntimeError` when the attribute does not exist or fails to be read.

## Examples

```python
alpha = target.get_attr("alpha")
```
