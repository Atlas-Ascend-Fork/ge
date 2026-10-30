# malloc\_output\_tensor

<!-- md-trans-meta sourceCommit=aed696e7709ab85c13269f21d417f4648a63a0b4 translatedAt=2026-10-15T02:35:23.007Z pushedAt=2026-10-16T10:53:07.323Z -->

## Applicable Products

Supported on all chips.

## Description

Applies for **Device** memory for an output **Tensor** and initializes the basic information of the output **Tensor**. The memory of the output **Tensor** is managed by the **Context** constructor. The API caller does not need to release it proactively.

## Prototype

```python
malloc_output_tensor(index: int, shape: StorageShape, format: StorageFormat, dtype: DataType) -> Tensor
```

## Parameters

| Parameter | Input/Output | Description |
| :--- | :--- |:----------------------|
| **index** | Input | Output index. |
| **shape** | Input | Shape of the output tensor. |
| **format** | Input | Format of the output tensor. |
| **dtype** | Input | Data type of the output tensor. |

## Return Value

| Type | Description |
| :--- |:---------------------------------------|
| **Tensor** | Output tensor. |

## Constraints

- This method can be called only within the current `execute` call stack.
- The returned `Tensor` can be used only within the current `execute`.
- `shape` and `format` must be valid storage description objects, and `index` must be within the output instance range of the current node. A `RuntimeError` is thrown if the allocation or initialization fails.

## Examples

```python
from ge.custom_op import get_execute_ctx


def execute(self, x) -> None:
    ctx = get_execute_ctx()
    output = ctx.malloc_output_tensor(0, x.shape, x.format, x.data_type)
```
