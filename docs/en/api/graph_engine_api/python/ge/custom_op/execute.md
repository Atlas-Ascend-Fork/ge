# execute

<!-- md-trans-meta sourceCommit=3bb5c44a2b84bf843658519f3b19948450f79744 translatedAt=2026-10-15T02:22:53.347Z pushedAt=2026-10-16T10:53:07.271Z -->

## Applicable Products

Supported on all chips.

## Description

Runtime execution callback of a Python custom operator. After the implementation class is registered through [register_op_impl](register_op_impl.md) and a callable `execute` method is provided, **GE** assembles the input and attribute parameters based on the operator prototype and invokes this method during the operator execution phase.

The `execute` callback does not receive output parameters, nor does it pass outputs through a return value. You can obtain `EagerOpExecutionContext` through [get_execute_ctx](get_execute_ctx.md) to allocate output memory or establish a Ref relationship between outputs and inputs.

## Prototype

The signature of the `execute` method follows the pattern below:

```python
def execute(self, input_0, ..., *, attr_0, ...) -> None
```

The parameter names above only indicate parameter positions. The actual number of parameters and attribute names are determined by the operator prototype.

## Parameters

| Parameter | Binding Rule                             |
| :--- |:-------------------------------------|
| Input parameter | Placed at the front of the parameter list. A required input is passed as `Tensor`, an optional input as `Optional[Tensor]`, and a dynamic input as `List[Tensor]`. |
| Attribute parameter | Placed after all input parameters and must use keyword-only parameters. The parameter name, order, and type must be consistent with the operator prototype. |

The parameter type is determined by the operator prototype. When a type annotation is provided for a parameter, the annotation must be consistent with the corresponding input or attribute type.

## Constraints

- `execute` can only be used in schema-bound form. The operator must have an operator prototype; otherwise, a `RuntimeError` is thrown during validation or invocation.
- The callback must not declare variable positional parameters or variable keyword parameters. A `TypeError` is thrown when the number, order, or attribute names of inputs and attributes do not match.
- `execute` does not require type hints for input parameters; however, any specified type hints are validated against the operator prototype to ensure consistency.
- The callback return value must be `None`, and the `-> None` return annotation must be declared.
- `get_execute_ctx()` can only be called within the current synchronous `execute` callback. The returned `EagerOpExecutionContext` and the borrowed objects such as `Tensor` returned by it can only be used within the current callback and become invalid after the callback returns or throws an exception.

## Examples

```python
from ge.custom_op import get_execute_ctx, register_op_impl
from ge.runtime import Tensor


@register_op_impl(op_type="AddPythonCustomOp")
class AddPythonCustomOp:
    def execute(self, x: Tensor, y: Tensor) -> None:
        ctx = get_execute_ctx()
        output = ctx.malloc_output_tensor(0, x.shape, x.format, x.data_type)
        # Use x, y, output, and ctx.get_stream() to launch the execution of the current operator.
```
