# add\_launch

<!-- md-trans-meta sourceCommit=6589d3ceddaf9347af65be52af6e83b97d48d142 translatedAt=2026-10-15T02:26:34.011Z pushedAt=2026-10-16T10:53:07.284Z -->

## Applicable Products

Supported on all chips.

## Description

Submits the metadata and parameter declarations of a **kernel launch**. After the call, GE records the parameter order of this launch for address refresh of the static model.

## Prototype

```python
add_launch(launch_info: AnnotatedKernelLaunchInfo, args: AnnotatedKernelArgs) -> None
```

## Parameters

| Parameter | Input/Output | Description |
| :--- | :--- | :--- |
| **launch_info** | Input | Kernel launch metadata of the type `AnnotatedKernelLaunchInfo`, including the kernel name, binary, block count, and stream identifier. |
| **args** | Input | Kernel argument constructor of the type `AnnotatedKernelArgs`. |

## Return Value

None

## Examples

```python
from ge.custom_op import AnnotatedKernelLaunchInfo, get_declare_launch_args_ctx
from ge.runtime import Tensor


def declare_launch_args(self, x1: Tensor, x2: Tensor, y: Tensor) -> None:
    ctx = get_declare_launch_args_ctx()
    args = ctx.create_kernel_args()
    args.append_input(0, x1)
    args.append_input(1, x2)
    args.append_output(0, y)
    kernel_bin = b"..."
    launch_info = AnnotatedKernelLaunchInfo(
        kernel_name="add_custom",
        kernel_bin=kernel_bin,
        block_dim=8,
        stream_id=ctx.get_stream_id(),
    )
    ctx.add_launch(launch_info, args)
```

## Constraints

- This method can only be called within the current `declare_launch_args` callback.
- `args` must be created by the current `AnnotatedArgsContext` through `create_kernel_args`. `args` is consumed only after `launch_info.stream_id` passes validation; after that, `args` must not be reused even if the underlying `AddLaunch` fails.
- `launch_info.stream_id` must equal the stream identifier of the current `AnnotatedArgsContext`. On mismatch, a `ValueError` is raised before `args` is consumed, at which point the builder has not yet been consumed and can continue to be used.
- A `RuntimeError` is raised when the underlying kernel launch addition fails, or when the current `AnnotatedArgsContext` or `args` has expired.
