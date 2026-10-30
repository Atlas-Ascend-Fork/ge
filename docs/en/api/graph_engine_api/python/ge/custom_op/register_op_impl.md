# register\_op\_impl

<!-- md-trans-meta sourceCommit=cb09f4dc9ab931756bc2831598909fac6d40548a translatedAt=2026-10-15T02:26:02.693Z pushedAt=2026-10-16T10:53:07.282Z -->

## Applicable Products

Supported on all chips.

## Description

Registers a Python custom operator implementation class. It supports implementing the `execute`, `compile`, and `declare_launch_args` methods, which can be used in combination.

## Prototype

```python
register_op_impl(*, op_type: str) -> callable
```

## Parameters

| Parameter | Input/Output | Description |
| :--- | :--- | :--- |
| op_type | Input | Custom operator type. It must be a non-empty string and unique in the implementation registry. |

## Constraints

- The registered implementation class must be a concrete class and must implement at least one supported method. For specific callback constraints, see [execute](execute.md), [compile](compile.md), and [declare_launch_args](declare_launch_args.md).
- `execute`, `compile`, and `declare_launch_args` must be defined as class methods on the implementation class and must be callable. If they are defined as non-callable objects, registration fails or the corresponding methods are not recognized.
- A `TypeError` is raised when `op_type` is invalid, the registered implementation class is not a concrete class, or the implementation class does not provide a supported method. A `ValueError` is raised when a conflict occurs due to duplicate registration of `op_type`.

## Examples

```python
from ge.custom_op import get_execute_ctx, register_op_impl
from ge.runtime import Tensor


@register_op_impl(op_type="AddCustom")
class AddCustom:
    def execute(self, x: Tensor, y: Tensor) -> None:
        ctx = get_execute_ctx()
        output = ctx.malloc_output_tensor(0, x.shape, x.format, x.data_type)
        # Initiate the execution of the current operator using x, y, output, and ctx.get_stream().
```
