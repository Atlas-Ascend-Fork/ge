# append\_output

<!-- md-trans-meta sourceCommit=6589d3ceddaf9347af65be52af6e83b97d48d142 translatedAt=2026-10-15T02:30:02.690Z pushedAt=2026-10-16T10:53:07.299Z -->

## Applicable Products

Supported on all chips.

## Description

Appends an output tensor parameter to the current kernel parameter sequence. The append position is determined by the call order.

## Prototype

```python
append_output(instance_index: int, tensor: Tensor) -> None
```

## Parameters

| Parameter | Input/Output | Description |
| :--- | :--- | :--- |
| instance_index | Input | Instance tiling index output by the current compute node. |
| tensor | Input | Output tensor to be appended. The caller should pass the tensor corresponding to this output in the current callback. |

## Return Value

None

## Examples

```python
from ge.custom_op import get_declare_launch_args_ctx
from ge.runtime import Tensor


def declare_launch_args(self, x1: Tensor, x2: Tensor, y: Tensor) -> None:
    ctx = get_declare_launch_args_ctx()
    args = ctx.create_kernel_args()
    args.append_output(0, y)
```

## Constraints

- `AnnotatedKernelArgs` can only be created by `create_kernel_args()` of the current `AnnotatedArgsContext`, and can only be used within the current `declare_launch_args` callback.
- `instance_index` must be within the instance tiling index range of the current compute node's output. An `IndexError` is raised when the index is out of range.
- When the current object has expired or has been consumed by `add_launch`, calling this method raises a `RuntimeError`.
