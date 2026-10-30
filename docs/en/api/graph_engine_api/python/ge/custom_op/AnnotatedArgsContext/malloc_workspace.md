# malloc\_workspace

<!-- md-trans-meta sourceCommit=6589d3ceddaf9347af65be52af6e83b97d48d142 translatedAt=2026-10-15T02:28:22.534Z pushedAt=2026-10-16T10:53:07.294Z -->

## Applicable Products

Supported on all chips.

## Description

Applies for a workspace for the current declarative parameter context and returns the address information of the workspace. The returned `WorkspaceAddr` can be passed to `AnnotatedKernelArgs.append_workspace` to declare kernel parameters.

## Prototype

```python
malloc_workspace(size: int) -> WorkspaceAddr
```

## Parameters

| Parameter | Input/Output | Description |
| :--- | :--- | :--- |
| size | Input | Workspace size, in bytes. The value must be greater than 0. |

## Return Value

| Type | Description |
| :--- | :--- |
| WorkspaceAddr | Workspace address information applied for by the current callback, including the workspace index and address. |

## Example

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

- This method can only be called within the current `declare_launch_args` callback. `WorkspaceAddr` is a borrowed object and can only be used within the current callback.
- A `RuntimeError` is raised when workspace allocation fails or the current `AnnotatedArgsContext` has expired.
