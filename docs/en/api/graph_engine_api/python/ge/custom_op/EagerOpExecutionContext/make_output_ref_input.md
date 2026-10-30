# make\_output\_ref\_input

<!-- md-trans-meta sourceCommit=412fcbc1686cc456fab1126c317e7c5811ddd410 translatedAt=2026-10-15T02:34:58.072Z pushedAt=2026-10-16T10:53:07.322Z -->

## Applicable Products

Supported on all chips.

## Description

Specifies that the memory address of a certain output references a certain input.

## Prototype

```python
make_output_ref_input(output_index: int, input_index: int) -> Tensor
```

## Parameters

| Parameter | Input/Output | Description |
| :--- | :--- | :--- |
| **output_index** | Input | Output index. |
| **input_index** | Input | Input index. |

## Return Value

| Type | Description |
| :--- |:-------------------------------|
| **Tensor** | The output tensor corresponding to **output_index**. |

## Constraints

- This method can be called only within the current `execute` call stack.
- The returned `Tensor` can be used only within the current `execute`.
- Both indexes must fall within the input and output instance ranges of the current node. A `RuntimeError` is thrown if the lookup or setting fails.
- The output corresponding to `output_index` and the input corresponding to `input_index` in this API must have their output-references-input relationship declared through [mutates_args](../register_op.md#parameters) in `register_op`. If the relationship is not declared through `mutates_args`, calling this API fails.

## Examples

The following example defines a simple operator prototype through **register_op**. **mutates_args=("x",)** indicates that the 0th output references the input **x**.

```python
from ge.custom_op import register_op
from ge.runtime import TensorDesc


@register_op(op_type="IdentityCustom", mutates_args=("x",))
def infer_meta(x: TensorDesc) -> TensorDesc:
    return x
```

In the **execute** implementation, use **make_output_ref_input(0, 0)** to set the 0th output to reference the memory address of the 0th input.

```python
from ge.custom_op import get_execute_ctx


def execute(self, x) -> None:
    ctx = get_execute_ctx()
    output = ctx.make_output_ref_input(0, 0)
```
