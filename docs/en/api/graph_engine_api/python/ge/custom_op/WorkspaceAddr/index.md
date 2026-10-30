# index

<!-- md-trans-meta sourceCommit=6589d3ceddaf9347af65be52af6e83b97d48d142 translatedAt=2026-10-15T02:39:32.863Z pushedAt=2026-10-16T10:53:07.340Z -->

## Applicable Products

Supported on all chips.

## Description

Obtains the instance index of the current **workspace**.

## Prototype

```python
@property
def index(self) -> int
```

## Parameters

None

## Return Value

| Type | Description |
| :--- | :--- |
| int | Instance index of the current workspace. |

## Examples

```python
from ge.custom_op import get_declare_launch_args_ctx


def declare_launch_args(self, x1, x2, y) -> None:
    ctx = get_declare_launch_args_ctx()
    workspace = ctx.malloc_workspace(256)
    workspace_index = workspace.index
```

## Constraints

- This is a read-only property. Access it using `workspace.index`; do not call it as `workspace.index()` or assign a value to it.
- `WorkspaceAddr` can only be returned by `AnnotatedArgsContext.malloc_workspace()` in the current `declare_launch_args` callback and cannot be constructed directly.
- After the current callback ends, this object becomes invalid along with `AnnotatedArgsContext`; accessing this property raises `RuntimeError`.
