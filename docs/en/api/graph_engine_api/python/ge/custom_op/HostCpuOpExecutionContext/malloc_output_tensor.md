# malloc\_output\_tensor

<!-- md-trans-meta sourceCommit=3bb5c44a2b84bf843658519f3b19948450f79744 translatedAt=2026-10-15T02:37:41.366Z pushedAt=2026-10-16T10:53:07.335Z -->

## Applicable Products

Supported on all chips.

## Description

Applies for Host memory for an output tensor and initializes the basic information of the output tensor.
The memory of the output tensor is managed by the Context constructor, and the caller does not need to release it.

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

- It can be called only within the current `execute` call stack.
- The returned `Tensor` can be used only within the current `execute`.
- `shape` and `format` must be valid storage description objects, and `index` must be within the output instance range of the current node. A `RuntimeError` is thrown if the allocation or initialization fails.

## Examples

```python
from ge.custom_op import get_execute_ctx


def execute(self, x) -> None:
    ctx = get_execute_ctx()
    output = ctx.malloc_output_tensor(0, x.shape, x.format, x.data_type)
```
