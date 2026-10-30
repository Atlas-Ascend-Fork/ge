# append\_workspace

<!-- md-trans-meta sourceCommit=6589d3ceddaf9347af65be52af6e83b97d48d142 translatedAt=2026-10-15T02:30:06.256Z pushedAt=2026-10-16T10:53:07.300Z -->

## Applicable Products

Supported on all chips.

## Description

Appends a **workspace** address parameter to the current **kernel** parameter sequence. The append position is determined by the call order.

## Prototype

```python
append_workspace(workspace: WorkspaceAddr) -> None
```

## Parameters

| Parameter | Input/Output | Description |
| :--- | :--- | :--- |
| workspace | Input | Workspace address information returned by `AnnotatedArgsContext.malloc_workspace()`. |

## Return Value

None

## Examples

```python
from ge.custom_op import get_declare_launch_args_ctx
from ge.runtime import Tensor


def declare_launch_args(self, x1: Tensor, x2: Tensor, y: Tensor) -> None:
    ctx = get_declare_launch_args_ctx()
    args = ctx.create_kernel_args()
    workspace = ctx.malloc_workspace(256)
    args.append_workspace(workspace)
```

## Constraints

- `AnnotatedKernelArgs` can only be created by `create_kernel_args()` of the current `AnnotatedArgsContext`, and can only be used within the current `declare_launch_args` callback.
- `WorkspaceAddr` is a borrowed object and can only be used within the `declare_launch_args` callback that obtains it.
- When the current object has expired or has been consumed by `add_launch`, calling this method throws `RuntimeError`.
