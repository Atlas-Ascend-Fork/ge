# declare\_launch\_args

<!-- md-trans-meta sourceCommit=1e3f578d326a470317eec72be531e70898c39984 translatedAt=2026-10-15T02:22:18.923Z pushedAt=2026-10-16T10:53:07.269Z -->

## Applicable Products

Supported on all chips.

## Description

Declarative kernel launch parameter callback for Python custom operators. After the implementation class is registered through [register_op_impl](register_op_impl.md) and a callable `declare_launch_args` method is provided, GE assembles the input, output, and attribute parameters based on the Ascend IR operator prototype and invokes this method during the compilation phase. The callback creates `AnnotatedKernelArgs` through [get_declare_launch_args_ctx](get_declare_launch_args_ctx.md), applies for workspace, and submits `AnnotatedKernelLaunchInfo`.

## Prototype

```python
def declare_launch_args(self, input_0, ..., output_0, ..., *, attr_0, ...) -> None
```

The parameter names above only indicate parameter positions. The actual number of parameters and attribute names are determined by the **Ascend IR** operator prototype of the operator.

## Parameters

GE binds parameters according to the Ascend IR operator prototype in the following order:

| Parameter | Binding Rule |
| :--- | :--- |
| Input parameters | Placed at the beginning of the parameter list. A required input is passed as `Tensor`, an optional input as `Optional[Tensor]`, and a dynamic input as `List[Tensor]`. |
| Output parameters | Placed after all input parameters. A required output is passed as `Tensor`, and a dynamic output as `List[Tensor]`. |
| Attribute parameters | Placed after all input and output parameters, and must use keyword-only parameters. The parameter names, order, and types are consistent with the Ascend IR operator prototype. |

The parameter types are determined by the Ascend IR operator prototype. When a parameter provides a type annotation, the annotation must be consistent with the corresponding input, output, or attribute type.

## Constraints

- `declare_launch_args` can only be used in schema-bound form. The operator must have an Ascend IR operator prototype; otherwise, a `RuntimeError` is thrown during validation or invocation.
- The callback must not declare variadic positional parameters or variadic keyword parameters. A `TypeError` is thrown when the number, order, or attribute names of inputs, outputs, and attributes do not match.
- The callback must return `None` and must declare the `-> None` return annotation.
- `get_declare_launch_args_ctx()` can only be called within the current synchronous `declare_launch_args` callback. The returned `AnnotatedArgsContext`, `AnnotatedKernelArgs`, `WorkspaceAddr`, and their derived borrowed objects can only be used within the current callback and become invalid after the callback returns or throws an exception.
- The `instance_index` of `AnnotatedKernelArgs.append_input` and `append_output` uses the instance-flattened index of the inputs and outputs of the current compute node; instances expanded from dynamic inputs or outputs use consecutive indices.
- After calling `AnnotatedArgsContext.add_launch`, the passed `AnnotatedKernelArgs` is consumed and cannot be used again.

## Examples

```python
from ge.custom_op import (
    AnnotatedKernelLaunchInfo,
    get_declare_launch_args_ctx,
    register_op_impl,
)
from ge.runtime import Tensor


kernel_bin = b"..."


@register_op_impl(op_type="AnnotatedAddCustom")
class AnnotatedAddCustom:
    def declare_launch_args(self, x1: Tensor, x2: Tensor, y: Tensor) -> None:
        ctx = get_declare_launch_args_ctx()
        args = ctx.create_kernel_args()
        args.append_input(0, x1)
        args.append_input(1, x2)
        args.append_output(0, y)
        ctx.add_launch(
            AnnotatedKernelLaunchInfo(
                kernel_name="add_custom",
                kernel_bin=kernel_bin,
                block_dim=8,
                stream_id=ctx.get_stream_id(),
            ),
            args,
        )
```
