# append\_scalar

<!-- md-trans-meta sourceCommit=6589d3ceddaf9347af65be52af6e83b97d48d142 translatedAt=2026-10-15T02:30:01.630Z pushedAt=2026-10-16T10:53:07.297Z -->

## Applicable Products

Supported on all chips.

## Description

Appends a 64-bit unsigned scalar parameter to the current kernel parameter sequence. The append position is determined by the call order.

## Prototype

```python
append_scalar(value: int) -> None
```

## Parameters

| Parameter | Input/Output | Description |
| :--- | :--- | :--- |
| value | Input | 64-bit unsigned scalar value, with a value range from 0 to 2^64-1. |

## Return Value

None

## Examples

```python
from ge.custom_op import get_declare_launch_args_ctx


def declare_launch_args(self, x1, x2, y) -> None:
    ctx = get_declare_launch_args_ctx()
    args = ctx.create_kernel_args()
    args.append_scalar(8)
```

## Constraints

- `AnnotatedKernelArgs` can only be created by `create_kernel_args()` of the current `AnnotatedArgsContext`, and can only be used within the current `declare_launch_args` callback.
- When the current object has expired or has been consumed by `add_launch`, calling this method throws `RuntimeError`.
