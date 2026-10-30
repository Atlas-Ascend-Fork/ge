# malloc\_workspace

<!-- md-trans-meta sourceCommit=aed696e7709ab85c13269f21d417f4648a63a0b4 translatedAt=2026-10-15T02:35:53.056Z pushedAt=2026-10-16T10:53:07.325Z -->

## Applicable Products

Supported on all chips.

## Description

Allocates **Workspace** memory with **Placement** set to **Device**. The memory is managed by the **Context** constructor, so the API caller does not need to release it proactively.

## Prototype

```python
malloc_workspace(size: int) -> int
```

## Parameters

| Parameter | Input/Output | Description |
| :--- | :--- | :--- |
| **size** | Input | Memory size, in bytes. |

## Return Value

| Type | Description |
| :--- | :--- |
| int | Device address of the **workspace**. |

## Constraints

- Can only be called within the current `execute` call stack.
- Throws `RuntimeError` when the allocation fails.

## Examples

```python
from ge.custom_op import get_execute_ctx


def execute(self, x) -> None:
    ctx = get_execute_ctx()
    workspace = ctx.malloc_workspace(256)
```
