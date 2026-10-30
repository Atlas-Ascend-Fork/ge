# get\_output\_tensor

<!-- md-trans-meta sourceCommit=aed696e7709ab85c13269f21d417f4648a63a0b4 translatedAt=2026-10-15T02:34:41.565Z pushedAt=2026-10-16T10:53:07.319Z -->

## Applicable Products

Supported on all chips.

## Description

Obtains the output tensor specified by **index**.

## Prototype

```python
get_output_tensor(index: int) -> Tensor
```

## Parameters

| Parameter | Input/Output | Description |
| :--- | :--- |:----------------------|
| **index** | Input | Output index. |

## Return Value

| Type | Description |
| :--- |:-------------|
| Tensor | Output tensor. |

## Constraints

- It can only be called within the current `execute` call stack.
- The returned `Tensor` can only be used within the current `execute`.
- `index` must be within the range of the output instances of the current compute node. A `RuntimeError` is thrown when the output is unavailable.

## Examples

```python
from ge.custom_op import get_execute_ctx


def execute(self, x) -> None:
    ctx = get_execute_ctx()
    output = ctx.get_output_tensor(0)
```
