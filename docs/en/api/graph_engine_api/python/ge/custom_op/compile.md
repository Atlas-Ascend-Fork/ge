# compile

<!-- md-trans-meta sourceCommit=1e3f578d326a470317eec72be531e70898c39984 translatedAt=2026-10-15T02:22:15.770Z pushedAt=2026-10-16T10:53:07.268Z -->

## Applicable Products

Supported on all chips.

## Description

A graph compilation-phase callback for Python custom operators. This callback depends on the Ascend IR operator prototype registered by GE. Native operators are registered through GE's `REG_OP`, while Python operator prototypes are synchronously registered by the bridge. The operator prototype defines the names, order, types, and input/output categories of inputs, outputs, and attributes. After the implementation class is registered through [register_op_impl](register_op_impl.md) and provides a callable `compile` method, GE invokes this method during the graph compilation phase. The callback can read the schema parameter and query the compilation environment and platform information through [get_compile_ctx](get_compile_ctx.md) and [get_compile_platform_info](get_compile_platform_info.md).

The `compile` callback is used only for graph compilation and is not invoked during model loading or model execution.

## Prototype

```python
def compile(self, input_0, ..., output_0, ..., *, attr_0, ...) -> None
```

The parameter names above only indicate parameter positions. The actual number of parameters and attribute names are determined by the Ascend IR operator prototype of the operator.

## Parameters

GE binds parameters according to the Ascend IR operator prototype in the following order:

| Parameter | Binding Rule |
| :--- | :--- |
| Input parameter | Located at the front of the parameter list. A required input is passed as `Tensor`, an optional input as `Optional[Tensor]`, and a dynamic input as `List[Tensor]`. |
| Output parameter | Located after all input parameters. A required output is passed as `Tensor`, and a dynamic output as `List[Tensor]`. |
| Attribute parameter | Located after all input and output parameters, and must use keyword-only parameters. The parameter names, order, and types are consistent with the Ascend IR operator prototype. |

The parameter types are determined by the Ascend IR operator prototype. When a parameter provides a type annotation, the annotation must be consistent with the corresponding input, output, or attribute type.

## Constraints

- `compile` can only be used in schema-bound form. The operator must have an Ascend IR operator prototype; otherwise, a `RuntimeError` is thrown during validation or invocation.
- The callback must not declare variadic positional parameters or variadic keyword parameters. A `TypeError` is thrown when the number, order, or attribute names of inputs, outputs, and attributes do not match.
- The callback return value must be `None`, and the `-> None` return annotation must be declared.
- `get_compile_ctx()` and `get_compile_platform_info()` can only be called within the current synchronous `compile` callback. The returned context, platform information, input/output `Tensor` objects, and Tensor attribute views are all borrowed objects and become invalid after the callback returns or throws an exception.
- Query results such as strings, integers, and `dict` in the compilation context are copies of Python values and can continue to be used after the callback ends.
- `compile` can be registered in combination with the `execute` or `declare_launch_args` capability on the same implementation class; the context and lifecycle of each callback are independent of one another.

## Examples

```python
from ge.custom_op import (
    get_compile_ctx,
    get_compile_platform_info,
    register_op_impl,
)
from ge.runtime import Tensor


@register_op_impl(op_type="AddCustom")
class AddCustom:
    def compile(self, x: Tensor, y: Tensor, z: Tensor, *, alpha: int) -> None:
        compile_ctx = get_compile_ctx()
        platform = get_compile_platform_info()
        option = compile_ctx.get_option("custom.compile.option")
        soc_version = platform.get_soc_version()
        core_num = platform.get_ai_core_num()
        # Complete the custom compilation logic based on the inputs, outputs, attributes, and compilation environment.
        _ = (x, y, z, alpha, option, soc_version, core_num)
```
