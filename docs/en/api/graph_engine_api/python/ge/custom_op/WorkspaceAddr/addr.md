# addr

<!-- md-trans-meta sourceCommit=6589d3ceddaf9347af65be52af6e83b97d48d142 translatedAt=2026-10-15T02:39:30.714Z pushedAt=2026-10-16T10:53:07.339Z -->

## Applicable Products

Supported on all chips.

## Description

Obtains the address integer of the workspace in the current compilation callback.

## Prototype

```python
@property
def addr(self) -> int
```

## Parameters

None

## Return Value

| Type | Description |
| :--- | :--- |
| int | Integer address of the workspace in the current compilation callback. |

## Examples

```python
from ge.custom_op import get_declare_launch_args_ctx


def declare_launch_args(self, x1, x2, y) -> None:
    ctx = get_declare_launch_args_ctx()
    workspace = ctx.malloc_workspace(256)
    workspace_addr = workspace.addr
```

## Constraints

- This property is read-only. Access it using `workspace.addr`; do not call `workspace.addr()` or assign a value to it.
- `WorkspaceAddr` can only be returned by `AnnotatedArgsContext.malloc_workspace()` in the current `declare_launch_args` callback and cannot be constructed directly.
- Python code must not dereference this address.
- After the current callback ends, this object becomes invalid along with `AnnotatedArgsContext`; accessing this property raises `RuntimeError`.
