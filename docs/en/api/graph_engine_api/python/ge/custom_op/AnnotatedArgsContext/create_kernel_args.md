# create\_kernel\_args

<!-- md-trans-meta sourceCommit=6589d3ceddaf9347af65be52af6e83b97d48d142 translatedAt=2026-10-15T02:27:24.447Z pushedAt=2026-10-16T10:53:07.288Z -->

## Applicable Products

Supported on all chips.

## Description

Creates an `AnnotatedKernelArgs` object for declaring the parameter order of a single kernel launch. You can use this object to append input, output, workspace, and scalar parameters, and then pass it to `add_launch` to submit the declaration.

## Prototype

```python
create_kernel_args() -> AnnotatedKernelArgs
```

## Parameters

None

## Return Value

| Type | Description |
| :--- | :--- |
| AnnotatedKernelArgs | The kernel parameter constructor created by the current **AnnotatedArgsContext**. |

## Examples

```python
from ge.custom_op import get_declare_launch_args_ctx
from ge.runtime import Tensor


def declare_launch_args(self, x1: Tensor, x2: Tensor, y: Tensor) -> None:
    ctx = get_declare_launch_args_ctx()
    args = ctx.create_kernel_args()
    args.append_input(0, x1)
    args.append_input(1, x2)
    args.append_output(0, y)
```

## Constraints

- This method can only be called within the current `declare_launch_args` callback. The returned `AnnotatedKernelArgs` is a borrowed object and can only be used within the current callback.
- After the returned `AnnotatedKernelArgs` is passed to the current `AnnotatedArgsContext.add_launch`, the constructor is consumed and must not be used again.
- A `RuntimeError` is raised when the current `AnnotatedArgsContext` has expired.
