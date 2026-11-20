# append\_input

<!-- md-trans-meta sourceCommit=6589d3ceddaf9347af65be52af6e83b97d48d142 translatedAt=2026-10-15T02:29:39.886Z pushedAt=2026-10-16T10:53:07.296Z -->

## Applicable Products

Supported on all chips.

## Description

Appends an input tensor parameter to the current kernel parameter sequence. The append position is determined by the call order.

## Prototype

```python
append_input(instance_index: int, tensor: Tensor) -> None
```

## Parameters

| Parameter | Input/Output | Description |
| :--- | :--- | :--- |
| instance_index | Input | Instance tiling index of the current compute node input. |
| tensor | Input | Input tensor to append. The caller should pass the tensor corresponding to this input in the current callback. |

## Return Value

None

## Examples

```python
from ge.custom_op import get_declare_launch_args_ctx
from ge.runtime import Tensor


def declare_launch_args(self, x1: Tensor, x2: Tensor, y: Tensor) -> None:
    ctx = get_declare_launch_args_ctx()
    args = ctx.create_kernel_args()
    args.append_input(0, x1)
```

## Constraints

- `AnnotatedKernelArgs` can only be created by `create_kernel_args()` of the current `AnnotatedArgsContext`, and can only be used within the current `declare_launch_args` callback.
- `instance_index` must be within the instance tiling index range of the current compute node's input. An `IndexError` is thrown when the index is out of range.
- When the current object has expired or has been consumed by `add_launch`, calling this method throws a `RuntimeError`.
