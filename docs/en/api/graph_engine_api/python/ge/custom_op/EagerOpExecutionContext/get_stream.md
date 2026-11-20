# get\_stream

<!-- md-trans-meta sourceCommit=aed696e7709ab85c13269f21d417f4648a63a0b4 translatedAt=2026-10-15T02:34:49.849Z pushedAt=2026-10-16T10:53:07.321Z -->

## Applicable Products

Supported on all chips.

## Description

Obtains the runtime execution stream handle bound to the current operator.

## Prototype

```python
get_stream() -> int
```

## Parameters

None

## Return Value

| Type | Description |
| :--- | :--- |
| int | Integer representation of the current runtime execution stream handle. |

## Constraints

- It can only be called within the current `execute` call stack.
- A `RuntimeError` is thrown when retrieval fails.

## Examples

```python
from ge.custom_op import get_execute_ctx


def execute(self, x) -> None:
    ctx = get_execute_ctx()
    stream = ctx.get_stream()
```
