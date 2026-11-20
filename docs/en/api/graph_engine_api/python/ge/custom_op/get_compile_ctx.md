# get\_compile\_ctx

<!-- md-trans-meta sourceCommit=1e3f578d326a470317eec72be531e70898c39984 translatedAt=2026-10-15T02:23:27.189Z pushedAt=2026-10-16T10:53:07.273Z -->

## Applicable Products

Supported on all chips.

## Description

Obtains the read-only compilation context of the current [compile](compile.md) callback.

## Prototype

```python
get_compile_ctx() -> OpCompileContext
```

## Parameters

None

## Constraints

- This API can be called only within the current synchronous `compile` callback. Calling it outside the callback raises `RuntimeError`.
- The returned object, as well as the `Tensor` objects and Tensor attributes obtained through the schema parameter, are borrowed views and become invalid after the callback returns or raises an exception.
- Query results such as strings, integers, and `dict` are copies of Python values and can continue to be used after the callback ends.
- This API is used only during the graph compilation phase and must not be called during model loading or execution.

## Examples

```python
from ge.custom_op import get_compile_ctx


def compile(self, x, y, *, alpha: int) -> None:
    ctx = get_compile_ctx()
    option = ctx.get_option("custom.compile.option")
```
