# get\_declare\_launch\_args\_ctx

<!-- md-trans-meta sourceCommit=1e3f578d326a470317eec72be531e70898c39984 translatedAt=2026-10-15T02:24:29.791Z pushedAt=2026-10-16T10:53:07.276Z -->

## Applicable Products

Supported on all chips.

## Description

Obtains the declarative argument context of the current [declare_launch_args](declare_launch_args.md) callback. Through the returned `AnnotatedArgsContext`, you can apply for logical workspace, obtain the stream identifier, create `AnnotatedKernelArgs`, and submit `AnnotatedKernelLaunchInfo` and kernel arguments.

## Prototype

```python
get_declare_launch_args_ctx() -> AnnotatedArgsContext
```

## Parameters

None

## Constraints

- This API can only be called within the current `declare_launch_args` callback. Calling it outside the callback raises `RuntimeError`.
- `AnnotatedArgsContext` is a borrowed object and is valid only within the current callback. The `AnnotatedKernelArgs` created from it and the `WorkspaceAddr` obtained from it can also be used only within the current callback.
- `AnnotatedKernelLaunchInfo` stores the kernel name, binary, block count, and stream identifier. Calling `AnnotatedArgsContext.add_launch` consumes the passed-in `AnnotatedKernelArgs`.
- The `instance_index` of `AnnotatedKernelArgs.append_input` and `append_output` uses the instance tiling index of the current compute node's input and output, respectively.

## Examples

```python
from ge.custom_op import get_declare_launch_args_ctx


def declare_launch_args(self, x1, x2, y) -> None:
    ctx = get_declare_launch_args_ctx()
    args = ctx.create_kernel_args()
```
