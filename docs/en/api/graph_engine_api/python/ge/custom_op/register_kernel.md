# register\_kernel

<!-- md-trans-meta sourceCommit=3bb5c44a2b84bf843658519f3b19948450f79744 translatedAt=2026-10-15T02:25:37.445Z pushedAt=2026-10-16T10:53:07.281Z -->

## Applicable Products

Supported on all chips.

## Description

Declares the execution **backend** for the **execute** method in the **register_op_impl** implementation class. When multiple **execute** methods with the same name are defined in the same class, the decorator overwrites the previously saved function object with the later definition.

## Prototype

```python
register_kernel(*, backend: OpBackend) -> callable
```

## Parameters

| Parameter | Input/Output | Description |
| :--- | :--- | :--- |
| backend | Input | [OpBackend](OpBackend.md) enumeration member. |

## Constraints

- It can only decorate the **execute** method in an implementation class decorated by **@register_op_impl**, and supports instance methods, **staticmethod**, and **classmethod**. When combined with the latter two, **register_kernel** must be placed at the outermost level.
- An implementation class can declare at most one **execute** for the same **backend**. When declaring **execute** for multiple backends separately, each **execute** must use **register_kernel** and must not be mixed with undecorated implementations.

## Examples

```python
from ge.custom_op import OpBackend, register_kernel, register_op_impl


@register_op_impl(op_type="MyCustomOp")
class MyCustomOpImpl:
    @register_kernel(backend=OpBackend.DEVICE)
    def execute(self, x, *, alpha: float) -> None:
        ...

    @register_kernel(backend=OpBackend.HOST)
    def execute(self, x, *, alpha: float) -> None:
        ...
```
