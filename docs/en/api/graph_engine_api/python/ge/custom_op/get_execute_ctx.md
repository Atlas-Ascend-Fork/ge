# get\_execute\_ctx

<!-- md-trans-meta sourceCommit=3bb5c44a2b84bf843658519f3b19948450f79744 translatedAt=2026-10-15T02:24:47.285Z pushedAt=2026-10-16T10:53:07.278Z -->

## Applicable Products

Supported on all chips.

## Description

Obtains the runtime execution context of the current `execute`.

## Prototype

```python
get_execute_ctx() -> EagerOpExecutionContext | HostCpuOpExecutionContext
```

## Parameters

None

## Return Value

| Type | Description |
| :--- | :--- |
| [EagerOpExecutionContext](EagerOpExecutionContext/overview.md) | Execution context of the device kernel. |
| [HostCpuOpExecutionContext](HostCpuOpExecutionContext/overview.md) | Execution context of the host kernel. |

## Examples

```python
from ge.custom_op import get_execute_ctx, register_op_impl
from ge.runtime import Tensor


@register_op_impl(op_type="AddPythonCustomOp")
class AddPythonCustomOp:
    def execute(self, x: Tensor, y: Tensor) -> None:
        ctx = get_execute_ctx()
        output = ctx.malloc_output_tensor(0, x.shape, x.format, x.data_type)
```

## Constraints

- Can only be called within the current `execute` call stack.
- The returned execution context can only be used within the current `execute`; the `Tensor` returned by it can also only be used within the current `execute`.
