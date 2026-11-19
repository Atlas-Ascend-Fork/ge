# get\_output\_tensor

<!-- md-trans-meta sourceCommit=3bb5c44a2b84bf843658519f3b19948450f79744 translatedAt=2026-10-15T02:36:47.609Z pushedAt=2026-10-16T10:53:07.329Z -->

## Applicable Products

Supported on all chips.

## Description

Obtains the output tensor specified by **index**.

## Prototype

```python
get_output_tensor(index: int) -> Tensor
```

## Parameters

| Parameter | Input/Output | Description                  |
| :--- | :--- |:----------------------|
| **index** | Input | Output index. |

## Return Value

| Type | Description |
| :--- |:-------------|
| **Tensor** | Output tensor. |

## Constraints

- It can only be called within the current `execute` call stack.
- The returned `Tensor` can only be used within the current `execute`.
- `index` must be within the range of the output instances of the current compute node. If the output is unavailable, a `RuntimeError` is raised.

## Examples

```python
from ge.custom_op import get_execute_ctx


def execute(self, x) -> None:
    ctx = get_execute_ctx()
    output = ctx.get_output_tensor(0)
```
