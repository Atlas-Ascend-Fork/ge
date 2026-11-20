# get\_stream\_id

<!-- md-trans-meta sourceCommit=6589d3ceddaf9347af65be52af6e83b97d48d142 translatedAt=2026-10-15T02:28:19.588Z pushedAt=2026-10-16T10:53:07.290Z -->

## Applicable Products

Supported on all chips.

## Description

Obtains the stream identifier of the current `AnnotatedArgsContext`. This identifier can be used to set `stream_id` when creating `AnnotatedKernelLaunchInfo`.

## Prototype

```python
get_stream_id() -> int
```

## Parameters

None

## Return Value

| Type | Description |
| :--- | :--- |
| int | Stream identifier of the current `AnnotatedArgsContext`. |

## Examples

```python
from ge.custom_op import AnnotatedKernelLaunchInfo, get_declare_launch_args_ctx


def declare_launch_args(self, x1, x2, y) -> None:
    ctx = get_declare_launch_args_ctx()
    stream_id = ctx.get_stream_id()
    kernel_bin = b"..."
    launch_info = AnnotatedKernelLaunchInfo(
        kernel_name="add_custom",
        kernel_bin=kernel_bin,
        block_dim=8,
        stream_id=stream_id,
    )
```

## Constraints

- This method can only be called within the current `declare_launch_args` callback.
- Throws `RuntimeError` when the current `AnnotatedArgsContext` has expired.
