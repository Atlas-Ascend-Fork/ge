# register\_op\_impl

<!-- md-trans-meta sourceCommit=184dd43cc1ac401fd5d103df2d9c0e00e33ecc86 translatedAt=2026-11-17T02:40:08.989Z pushedAt=2026-11-18T02:41:08.865Z nl=2 -->

## Applicable Products

Supported on all chips.

## Description

Registers the implementation class of a Python custom operator. It supports implementing the `execute`, `compile`, `declare_launch_args`, and paired `serialize`/`deserialize` methods, which can be used in combination.

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
- `serialize` and `deserialize` must be declared in pairs in the implementation class itself. Methods with the same name inherited from the base class are not identified. If only one of them is declared, a `TypeError` is thrown, and the error message contains the operator type and the name of the missing method.

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
